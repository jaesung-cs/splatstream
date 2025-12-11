#include "vkgs/core/renderer.h"

#include <cstring>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/cmd/barrier.h"
#include "vkgs/gpu/cmd/pipeline.h"
#include "vkgs/gpu/image.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/compute_pipeline.h"
#include "vkgs/gpu/graphics_pipeline.h"
#include "vkgs/gpu/timer.h"
#include "vkgs/gpu/task.h"

#include "vkgs/core/gaussian_splats.h"
#include "vkgs/core/rendering_task.h"
#include "vkgs/core/screen_splats.h"
#include "generated/rank.h"
#include "generated/inverse_index.h"
#include "generated/projection.h"
#include "generated/splat_color_vert.h"
#include "generated/splat_color_frag.h"
#include "generated/splat_depth_vert.h"
#include "generated/splat_depth_frag.h"
#include "struct.h"

namespace {

auto WorkgroupSize(size_t count, uint32_t local_size) { return (count + local_size - 1) / local_size; }

}  // namespace

namespace vkgs {
namespace core {

RendererImpl::RendererImpl() {
  auto device = gpu::GetDevice();
  sorter_ = Sorter::Create(device, device->physical_device());

  device_name_ = device->device_name();
  graphics_queue_index_ = device->graphics_queue_index();
  compute_queue_index_ = device->compute_queue_index();
  transfer_queue_index_ = device->transfer_queue_index();

  for (auto& buffer : ring_buffer_) {
    buffer.compute_storage = ComputeStorage::Create();
    buffer.screen_splats = ScreenSplats::Create();
    buffer.graphics_storage = GraphicsStorage::Create();
    buffer.compute_semaphore = device->AllocateSemaphore();
    buffer.graphics_semaphore = device->AllocateSemaphore();
    buffer.transfer_semaphore = device->AllocateSemaphore();
  }

  compute_pipeline_layout_ = gpu::PipelineLayout::Create({
      .bindings =
          {
              {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
              {9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
          },
      .push_constants = {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ProjectionPushConstants)}},
  });
  rank_pipeline_ = gpu::ComputePipeline::Create(compute_pipeline_layout_, rank);
  inverse_index_pipeline_ = gpu::ComputePipeline::Create(compute_pipeline_layout_, inverse_index);
  projection_pipeline_ = gpu::ComputePipeline::Create(compute_pipeline_layout_, projection);

  graphics_pipeline_layout_ = gpu::PipelineLayout::Create({
      .bindings = {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}},
      .push_constants = {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SplatPushConstants)}},
  });
}

RendererImpl::~RendererImpl() = default;

RenderingTask RendererImpl::Draw(GaussianSplats splats, const DrawOptions& draw_options,
                                 const ScreenSplatOptions& screen_splat_options, uint8_t* dst) {
  auto rendering_task = RenderingTask::Create();

  uint32_t width = draw_options.width;
  uint32_t height = draw_options.height;

  auto N = splats->size();

  // Update storages
  const auto& ring_buffer = ring_buffer_[frame_index_ % ring_buffer_.size()];
  auto compute_storage = ring_buffer.compute_storage;
  auto screen_splats = ring_buffer.screen_splats;
  auto graphics_storage = ring_buffer.graphics_storage;
  auto csem = ring_buffer.compute_semaphore;
  auto cval = csem->value();
  auto gsem = ring_buffer.graphics_semaphore;
  auto gval = gsem->value();
  auto tsem = ring_buffer.transfer_semaphore;
  auto tval = tsem->value();

  auto cq = compute_queue_index_;
  auto gq = graphics_queue_index_;
  auto tq = transfer_queue_index_;

  screen_splats->Update(N);
  graphics_storage->Update(width, height);

  auto timer = gpu::Timer::Create(3);

  // Compute queue
  {
    gpu::ComputeTask task;
    auto cb = task.command_buffer();

    // Compute
    ComputeScreenSplats(cb, splats, draw_options, screen_splats, timer);

    // Release
    gpu::cmd::Barrier()
        .Release(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, cq, gq,
                 screen_splats->instances())
        .Release(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, cq, gq,
                 screen_splats->draw_indirect())
        .Commit(cb);

    // G[i-2].read before C[i].comp
    task.WaitIf(gval >= 2, gsem, gval - 2 + 1,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    // C[i].comp
    task.Signal(csem, cval + 1, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
  }

  auto image = graphics_storage->image();
  auto image_u8 = graphics_storage->image_u8();

  // Graphics queue
  {
    gpu::GraphicsTask task;
    auto cb = task.command_buffer();

    // Acquire
    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, cq, gq, screen_splats->instances())
        .Acquire(VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, cq, gq,
                 screen_splats->draw_indirect())
        .Commit(cb);

    // Layout transition to color attachment
    gpu::cmd::Barrier()
        .Image(0, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
               VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, image)
        .Commit(cb);

    // Rendering
    VkRenderingAttachmentInfo color_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = image->image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {draw_options.background.r, draw_options.background.g, draw_options.background.b, 0.f},
    };
    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, {width, height}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
    };
    vkCmdBeginRendering(cb, &rendering_info);

    VkViewport viewport = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
    vkCmdSetViewport(cb, 0, 1, &viewport);
    VkRect2D scissor = {0, 0, width, height};
    vkCmdSetScissor(cb, 0, 1, &scissor);

    RenderTargetOptions render_target_options = {
        .formats = {image->format()},
        .locations = {0},
    };
    RenderScreenSplatsColor(cb, screen_splats, screen_splat_options, render_target_options);

    vkCmdEndRendering(cb);

    // float -> uint8
    gpu::cmd::Barrier()
        .Image(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
               VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image)
        .Image(0, 0, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_u8)
        .Commit(cb);

    VkImageBlit image_region = {
        .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .srcOffsets = {{0, 0, 0}, {static_cast<int>(width), static_cast<int>(height), 1}},
        .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .dstOffsets = {{0, 0, 0}, {static_cast<int>(width), static_cast<int>(height), 1}},
    };
    vkCmdBlitImage(cb, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image_u8, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                   &image_region, VK_FILTER_NEAREST);

    timer->Record(cb, VK_PIPELINE_STAGE_2_BLIT_BIT);

    // Layout transition to transfer src, and release
    gpu::cmd::Barrier()
        .Release(VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, gq, tq, image_u8)
        .Commit(cb);

    // C[i].comp before G[i].read
    task.Wait(csem, cval + 1,
              VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                  VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);
    // T[i-2].xfer before G[i].output
    task.Wait(tsem, tval - 1 + 1, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    // G[i].read
    task.Signal(gsem, gval + 1,
                VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);
    // G[i].blit
    task.Signal(gsem, gval + 2, VK_PIPELINE_STAGE_2_BLIT_BIT);
  }

  auto image_buffer = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT, width * height * 4, true);
  gpu::QueueTask queue_task;
  {
    gpu::TransferTask task;
    auto cb = task.command_buffer();

    gpu::cmd::Barrier()
        .Acquire(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, gq, tq, image_u8)
        .Commit(cb);

    // Image to buffer
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1},
    };
    vkCmdCopyImageToBuffer(cb, image_u8, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image_buffer, 1, &region);

    timer->Record(cb, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    task.PostCallback([width, height, image_buffer, dst, timer, rendering_task] {
      std::memcpy(dst, image_buffer->data<uint8_t>(), width * height * 4);

      auto timestamps = timer->GetTimestamps();
      DrawResult draw_result = {
          .compute_timestamp = timestamps[0],
          .graphics_timestamp = timestamps[1],
          .transfer_timestamp = timestamps[2],
      };
      rendering_task->SetDrawResult(draw_result);
    });

    // G[i].blit before T[i].xfer
    task.Wait(gsem, gval + 2, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    // T[i].xfer
    task.Signal(tsem, tval + 1, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    queue_task = task.Submit();
  }

  rendering_task->SetTask(queue_task);

  csem->Increment();
  gsem->Increment();
  gsem->Increment();
  tsem->Increment();
  frame_index_++;

  return rendering_task;
}

void RendererImpl::ComputeScreenSplats(VkCommandBuffer cb, GaussianSplats splats, const DrawOptions& draw_options,
                                       ScreenSplats screen_splats, gpu::Timer timer) {
  auto N = splats->size();
  auto position = splats->position();
  auto cov3d = splats->cov3d();
  auto sh = splats->sh();
  auto opacity = splats->opacity();

  const auto& ring_buffer = ring_buffer_[frame_index_ % ring_buffer_.size()];
  auto compute_storage = ring_buffer.compute_storage;
  auto requirements = sorter_->GetStorageRequirements(N);
  compute_storage->Update(N, requirements.usage, requirements.size);

  auto key = compute_storage->key();
  auto index = compute_storage->index();
  auto sort_storage = compute_storage->sort_storage();
  auto inverse_index = compute_storage->inverse_index();
  auto camera = compute_storage->camera();
  auto camera_stage = compute_storage->camera_stage();

  auto visible_point_count = screen_splats->visible_point_count();
  auto draw_indirect = screen_splats->draw_indirect();
  auto instances = screen_splats->instances();
  auto stats = screen_splats->stats();

  ProjectionPushConstants projection_push_constants = {
      .model = draw_options.model,
      .point_count = static_cast<uint32_t>(N),
      .eps2d = draw_options.eps2d,
      .sh_degree_data = splats->sh_degree(),
      .sh_degree_draw = draw_options.sh_degree == -1 ? splats->sh_degree() : draw_options.sh_degree,
      .record_stat = draw_options.record_stat,
  };

  Camera camera_data = {
      .projection = draw_options.projection,
      .view = draw_options.view,
      .camera_position = glm::inverse(draw_options.view)[3],
      .screen_size = glm::uvec2(draw_options.width, draw_options.height),
  };
  std::memcpy(camera_stage->data(), &camera_data, sizeof(Camera));

  VkBufferCopy region = {0, 0, sizeof(Camera)};
  vkCmdCopyBuffer(cb, camera_stage, camera, 1, &region);
  vkCmdFillBuffer(cb, visible_point_count, 0, sizeof(uint32_t), 0);
  vkCmdFillBuffer(cb, inverse_index, 0, N * sizeof(uint32_t), -1);
  if (draw_options.record_stat) vkCmdFillBuffer(cb, stats, 0, stats->size(), 0);

  gpu::cmd::Barrier()
      .Memory(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
              VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT)
      .Commit(cb);

  // Rank
  gpu::cmd::Pipeline pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout_);
  pipeline.Storage(0, camera)
      .Storage(1, position)
      .Storage(2, visible_point_count)
      .Storage(3, key)
      .Storage(4, index)
      .PushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(projection_push_constants), &projection_push_constants)
      .Bind(rank_pipeline_)
      .Commit(cb);
  vkCmdDispatch(cb, WorkgroupSize(N, 256), 1, 1);

  // Sort
  gpu::cmd::Barrier()
      .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
              VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
              VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_TRANSFER_READ_BIT)
      .Commit(cb);

  sorter_->SortKeyValueIndirect(cb, N, visible_point_count, key, index, sort_storage);

  // Inverse index
  gpu::cmd::Barrier()
      .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
              VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
      .Commit(cb);

  pipeline.Storage(0, visible_point_count)
      .Storage(1, index)
      .Storage(2, inverse_index)
      .PushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(projection_push_constants), &projection_push_constants)
      .Bind(inverse_index_pipeline_)
      .Commit(cb);
  vkCmdDispatch(cb, WorkgroupSize(N, 256), 1, 1);

  // Projection
  gpu::cmd::Barrier()
      .Memory(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
              VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT)
      .Commit(cb);

  pipeline.Storage(0, camera)
      .Storage(1, position)
      .Storage(2, cov3d)
      .Storage(3, opacity)
      .Storage(4, sh)
      .Storage(5, visible_point_count)
      .Storage(6, inverse_index)
      .Storage(7, draw_indirect)
      .Storage(8, instances)
      .Storage(9, stats)
      .Bind(projection_pipeline_)
      .Commit(cb);
  vkCmdDispatch(cb, WorkgroupSize(N, 256), 1, 1);

  if (timer) {
    timer->Record(cb, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
  }

  if (draw_options.record_stat) {
  }

  visible_point_count->Keep();
  key->Keep();
  index->Keep();
  sort_storage->Keep();
  inverse_index->Keep();
  camera->Keep();
  camera_stage->Keep();
  draw_indirect->Keep();
  instances->Keep();
  if (draw_options.record_stat) stats->Keep();

  screen_splats->SetIndexBuffer(splats->index_buffer());
  screen_splats->SetProjection(draw_options.projection);
}

void RendererImpl::RenderScreenSplatsColor(VkCommandBuffer cb, ScreenSplats screen_splats,
                                           const ScreenSplatOptions& screen_splat_options,
                                           const RenderTargetOptions& render_target_options) {
  auto splat_color_pipeline = gpu::GraphicsPipeline::Create({
      .pipeline_layout = graphics_pipeline_layout_,
      .vertex_shader = gpu::ShaderCode(splat_color_vert),
      .fragment_shader = gpu::ShaderCode(splat_color_frag),
      .formats = render_target_options.formats,
      .locations = render_target_options.locations,
      .depth_format = render_target_options.depth_format,
      .depth_test = render_target_options.depth_format != VK_FORMAT_UNDEFINED,
  });

  SplatPushConstants splat_push_constants = {
      .confidence_radius = screen_splat_options.confidence_radius,
  };
  gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout_)
      .PushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(splat_push_constants), &splat_push_constants)
      .Storage(0, screen_splats->instances())
      .Bind(splat_color_pipeline)
      .Commit(cb);

  vkCmdBindIndexBuffer(cb, screen_splats->index_buffer(), 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexedIndirect(cb, screen_splats->draw_indirect(), 0, 1, 0);

  splat_color_pipeline->Keep();
  screen_splats->index_buffer()->Keep();
  screen_splats->instances()->Keep();
  screen_splats->draw_indirect()->Keep();
}

void RendererImpl::RenderScreenSplatsDepth(VkCommandBuffer cb, ScreenSplats screen_splats,
                                           const ScreenSplatOptions& screen_splat_options,
                                           const RenderTargetOptions& render_target_options) {
  auto splat_depth_pipeline = gpu::GraphicsPipeline::Create({
      .pipeline_layout = graphics_pipeline_layout_,
      .vertex_shader = gpu::ShaderCode(splat_depth_vert),
      .fragment_shader = gpu::ShaderCode(splat_depth_frag),
      .formats = render_target_options.formats,
      .locations = render_target_options.locations,
      .depth_format = render_target_options.depth_format,
      .depth_test = render_target_options.depth_format != VK_FORMAT_UNDEFINED,
  });

  SplatPushConstants splat_push_constants = {
      .projection_inverse = glm::inverse(screen_splats->projection()),
      .confidence_radius = screen_splat_options.confidence_radius,
  };
  gpu::cmd::Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout_)
      .PushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(splat_push_constants), &splat_push_constants)
      .Storage(0, screen_splats->instances())
      .Bind(splat_depth_pipeline)
      .Commit(cb);

  vkCmdBindIndexBuffer(cb, screen_splats->index_buffer(), 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexedIndirect(cb, screen_splats->draw_indirect(), 0, 1, 0);

  splat_depth_pipeline->Keep();
  screen_splats->index_buffer()->Keep();
  screen_splats->instances()->Keep();
  screen_splats->draw_indirect()->Keep();
}

}  // namespace core
}  // namespace vkgs
