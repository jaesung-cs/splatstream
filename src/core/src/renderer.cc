#include "vkgs/core/renderer.h"

#include <cstring>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "volk.h"

#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/image.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/fence.h"
#include "vkgs/gpu/queue.h"
#include "vkgs/gpu/command.h"
#include "vkgs/gpu/task_monitor.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/compute_pipeline.h"
#include "vkgs/gpu/graphics_pipeline.h"

#include "vkgs/core/gaussian_splats.h"
#include "vkgs/core/rendered_image.h"
#include "generated/parse_ply.h"
#include "generated/parse_data.h"
#include "generated/rank.h"
#include "generated/inverse_index.h"
#include "generated/projection.h"
#include "generated/splat_vert.h"
#include "generated/splat_frag.h"
#include "generated/splat_background_vert.h"
#include "generated/splat_background_frag.h"
#include "sorter.h"
#include "compute_storage.h"
#include "graphics_storage.h"
#include "transfer_storage.h"
#include "struct.h"

namespace {

auto WorkgroupSize(size_t count, uint32_t local_size) { return (count + local_size - 1) / local_size; }

void cmdPushDescriptorSet(VkCommandBuffer cb, VkPipelineBindPoint bind_point, VkPipelineLayout pipeline_layout,
                          const std::vector<VkBuffer>& buffers) {
  std::vector<VkDescriptorBufferInfo> buffer_infos(buffers.size());
  std::vector<VkWriteDescriptorSet> writes(buffers.size());
  for (int i = 0; i < buffers.size(); ++i) {
    buffer_infos[i] = {buffers[i], 0, VK_WHOLE_SIZE};
    writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writes[i].dstBinding = i;
    writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[i].descriptorCount = 1;
    writes[i].pBufferInfo = &buffer_infos[i];
  }
  vkCmdPushDescriptorSet(cb, bind_point, pipeline_layout, 0, writes.size(), writes.data());
}

}  // namespace

namespace vkgs {
namespace core {

Renderer::Renderer() {
  device_ = std::make_shared<gpu::Device>();
  task_monitor_ = std::make_shared<gpu::TaskMonitor>();
  sorter_ = std::make_shared<Sorter>(*device_, device_->physical_device());

  for (int i = 0; i < 2; ++i) {
    auto& double_buffer = double_buffer_[i];
    double_buffer.compute_storage = std::make_shared<ComputeStorage>(device_);
    double_buffer.graphics_storage = std::make_shared<GraphicsStorage>(device_);
    double_buffer.transfer_storage = std::make_shared<TransferStorage>(device_);
    double_buffer.compute_semaphore = device_->AllocateSemaphore();
    double_buffer.graphics_semaphore = device_->AllocateSemaphore();
    double_buffer.transfer_semaphore = device_->AllocateSemaphore();
  }

  parse_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_,
                                  {
                                      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                  },
                                  {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ParsePushConstants)}});
  parse_ply_pipeline_ = gpu::ComputePipeline::Create(*device_, *parse_pipeline_layout_, parse_ply);
  parse_data_pipeline_ = gpu::ComputePipeline::Create(*device_, *parse_pipeline_layout_, parse_data);

  compute_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_,
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
                                  },
                                  {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants)}});
  rank_pipeline_ = gpu::ComputePipeline::Create(*device_, *compute_pipeline_layout_, rank);
  inverse_index_pipeline_ = gpu::ComputePipeline::Create(*device_, *compute_pipeline_layout_, inverse_index);
  projection_pipeline_ = gpu::ComputePipeline::Create(*device_, *compute_pipeline_layout_, projection);

  graphics_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_, {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}},
                                  {{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GraphicsPushConstants)}});
  splat_pipeline_ = gpu::GraphicsPipeline::Create(*device_, *graphics_pipeline_layout_, splat_vert, splat_frag,
                                                  VK_FORMAT_R32G32B32A32_SFLOAT);
  splat_background_pipeline_ =
      gpu::GraphicsPipeline::Create(*device_, *graphics_pipeline_layout_, splat_background_vert, splat_background_frag,
                                    VK_FORMAT_R32G32B32A32_SFLOAT);
}

Renderer::~Renderer() = default;

const std::string& Renderer::device_name() const noexcept { return device_->device_name(); }
uint32_t Renderer::graphics_queue_index() const noexcept { return device_->graphics_queue_index(); }
uint32_t Renderer::compute_queue_index() const noexcept { return device_->compute_queue_index(); }
uint32_t Renderer::transfer_queue_index() const noexcept { return device_->transfer_queue_index(); }

std::shared_ptr<GaussianSplats> Renderer::CreateGaussianSplats(size_t size, const float* means_ptr,
                                                               const float* quats_ptr, const float* scales_ptr,
                                                               const float* opacities_ptr, const uint16_t* colors_ptr,
                                                               int sh_degree) {
  std::vector<uint32_t> index_data;
  index_data.reserve(6 * size);
  for (int i = 0; i < size; ++i) {
    index_data.push_back(4 * i + 0);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 3);
  }

  int colors_size = 0;
  int sh_packed_size = 0;
  switch (sh_degree) {
    case 0:
      colors_size = 1;
      sh_packed_size = 1;
      break;
    case 1:
      colors_size = 4;
      sh_packed_size = 3;
      break;
    case 2:
      colors_size = 9;
      sh_packed_size = 7;
      break;
    case 3:
      colors_size = 16;
      sh_packed_size = 12;
      break;
    default:
      throw std::runtime_error("Unsupported SH degree: " + std::to_string(sh_degree));
  }

  auto position_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 3 * sizeof(float), true);
  auto quats_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 4 * sizeof(float), true);
  auto scales_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 3 * sizeof(float), true);
  auto colors_stage =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * colors_size * 3 * sizeof(uint16_t), true);
  auto opacity_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * sizeof(float), true);
  auto index_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size * 6 * sizeof(uint32_t), true);

  auto position = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                      size * 3 * sizeof(float));
  auto quats = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   size * 4 * sizeof(float));
  auto scales = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    size * 3 * sizeof(float));
  auto colors = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    size * colors_size * 3 * sizeof(uint16_t));
  auto opacity = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     size * sizeof(float));

  auto cov3d = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size * 6 * sizeof(float));
  auto sh =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size * sh_packed_size * 4 * sizeof(uint16_t));
  auto index_buffer = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          size * 6 * sizeof(uint32_t));

  std::memcpy(position_stage->data(), means_ptr, position_stage->size());
  std::memcpy(quats_stage->data(), quats_ptr, quats_stage->size());
  std::memcpy(scales_stage->data(), scales_ptr, scales_stage->size());
  std::memcpy(opacity_stage->data(), opacities_ptr, opacity_stage->size());
  std::memcpy(colors_stage->data(), colors_ptr, colors_stage->size());
  std::memcpy(index_stage->data(), index_data.data(), index_stage->size());

  ParsePushConstants parse_data_push_constants = {};
  parse_data_push_constants.point_count = size;
  parse_data_push_constants.sh_degree = sh_degree;

  auto sem = device_->AllocateSemaphore();
  auto tq = device_->transfer_queue();
  auto cq = device_->compute_queue();
  auto gq = device_->graphics_queue();

  std::shared_ptr<gpu::Task> task;

  // Transfer queue: stage to buffers
  {
    auto cb = tq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    VkBufferCopy region = {0, 0, position_stage->size()};
    vkCmdCopyBuffer(*cb, *position_stage, *position, 1, &region);
    region = {0, 0, quats_stage->size()};
    vkCmdCopyBuffer(*cb, *quats_stage, *quats, 1, &region);
    region = {0, 0, scales_stage->size()};
    vkCmdCopyBuffer(*cb, *scales_stage, *scales, 1, &region);
    region = {0, 0, colors_stage->size()};
    vkCmdCopyBuffer(*cb, *colors_stage, *colors, 1, &region);
    region = {0, 0, opacity_stage->size()};
    vkCmdCopyBuffer(*cb, *opacity_stage, *opacity, 1, &region);
    region = {0, 0, index_stage->size()};
    vkCmdCopyBuffer(*cb, *index_stage, *index_buffer, 1, &region);

    std::vector<VkBufferMemoryBarrier2> release_barriers(6);
    release_barriers[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barriers[0].srcQueueFamilyIndex = tq->family_index();
    release_barriers[0].dstQueueFamilyIndex = cq->family_index();
    release_barriers[0].buffer = *position;
    release_barriers[0].offset = 0;
    release_barriers[0].size = VK_WHOLE_SIZE;
    release_barriers[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barriers[1].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barriers[1].srcQueueFamilyIndex = tq->family_index();
    release_barriers[1].dstQueueFamilyIndex = cq->family_index();
    release_barriers[1].buffer = *quats;
    release_barriers[1].offset = 0;
    release_barriers[1].size = VK_WHOLE_SIZE;
    release_barriers[2] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barriers[2].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barriers[2].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barriers[2].srcQueueFamilyIndex = tq->family_index();
    release_barriers[2].dstQueueFamilyIndex = cq->family_index();
    release_barriers[2].buffer = *scales;
    release_barriers[2].offset = 0;
    release_barriers[2].size = VK_WHOLE_SIZE;
    release_barriers[3] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barriers[3].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barriers[3].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barriers[3].srcQueueFamilyIndex = tq->family_index();
    release_barriers[3].dstQueueFamilyIndex = cq->family_index();
    release_barriers[3].buffer = *colors;
    release_barriers[3].offset = 0;
    release_barriers[3].size = VK_WHOLE_SIZE;
    release_barriers[4] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barriers[4].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barriers[4].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barriers[4].srcQueueFamilyIndex = tq->family_index();
    release_barriers[4].dstQueueFamilyIndex = cq->family_index();
    release_barriers[4].buffer = *opacity;
    release_barriers[4].offset = 0;
    release_barriers[4].size = VK_WHOLE_SIZE;
    release_barriers[5] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barriers[5].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barriers[5].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barriers[5].srcQueueFamilyIndex = tq->family_index();
    release_barriers[5].dstQueueFamilyIndex = gq->family_index();
    release_barriers[5].buffer = *index_buffer;
    release_barriers[5].offset = 0;
    release_barriers[5].size = VK_WHOLE_SIZE;
    VkDependencyInfo release_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    release_dependency_info.bufferMemoryBarrierCount = release_barriers.size();
    release_dependency_info.pBufferMemoryBarriers = release_barriers.data();
    vkCmdPipelineBarrier2(*cb, &release_dependency_info);

    vkEndCommandBuffer(*cb);

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_info.semaphore = *sem;
    signal_semaphore_info.value = sem->value() + 1;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal_semaphore_info;

    vkQueueSubmit2(*tq, 1, &submit, *fence);
    task_monitor_->Add(fence, {cb, position_stage, quats_stage, scales_stage, colors_stage, opacity_stage, index_stage,
                               position, quats, scales, colors, opacity, index_buffer});
  }

  // Compute queue: parse data
  {
    auto cb = cq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    std::vector<VkBufferMemoryBarrier2> acquire_barriers(5);
    acquire_barriers[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    acquire_barriers[0].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    acquire_barriers[0].srcQueueFamilyIndex = tq->family_index();
    acquire_barriers[0].dstQueueFamilyIndex = cq->family_index();
    acquire_barriers[0].buffer = *position;
    acquire_barriers[0].offset = 0;
    acquire_barriers[0].size = VK_WHOLE_SIZE;
    acquire_barriers[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    acquire_barriers[1].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    acquire_barriers[1].srcQueueFamilyIndex = tq->family_index();
    acquire_barriers[1].dstQueueFamilyIndex = cq->family_index();
    acquire_barriers[1].buffer = *quats;
    acquire_barriers[1].offset = 0;
    acquire_barriers[1].size = VK_WHOLE_SIZE;
    acquire_barriers[2] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barriers[2].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    acquire_barriers[2].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    acquire_barriers[2].srcQueueFamilyIndex = tq->family_index();
    acquire_barriers[2].dstQueueFamilyIndex = cq->family_index();
    acquire_barriers[2].buffer = *scales;
    acquire_barriers[2].offset = 0;
    acquire_barriers[2].size = VK_WHOLE_SIZE;
    acquire_barriers[3] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barriers[3].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    acquire_barriers[3].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    acquire_barriers[3].srcQueueFamilyIndex = tq->family_index();
    acquire_barriers[3].dstQueueFamilyIndex = cq->family_index();
    acquire_barriers[3].buffer = *colors;
    acquire_barriers[3].offset = 0;
    acquire_barriers[3].size = VK_WHOLE_SIZE;
    acquire_barriers[4] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barriers[4].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    acquire_barriers[4].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    acquire_barriers[4].srcQueueFamilyIndex = tq->family_index();
    acquire_barriers[4].dstQueueFamilyIndex = cq->family_index();
    acquire_barriers[4].buffer = *opacity;
    acquire_barriers[4].offset = 0;
    acquire_barriers[4].size = VK_WHOLE_SIZE;
    VkDependencyInfo acquire_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    acquire_dependency_info.bufferMemoryBarrierCount = acquire_barriers.size();
    acquire_dependency_info.pBufferMemoryBarriers = acquire_barriers.data();
    vkCmdPipelineBarrier2(*cb, &acquire_dependency_info);

    cmdPushDescriptorSet(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *parse_pipeline_layout_,
                         {*quats, *scales, *cov3d, *colors, *sh});
    vkCmdPushConstants(*cb, *parse_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parse_data_push_constants),
                       &parse_data_push_constants);
    vkCmdBindPipeline(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *parse_data_pipeline_);
    vkCmdDispatch(*cb, WorkgroupSize(size, 256), 1, 1);

    VkMemoryBarrier2 memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    vkEndCommandBuffer(*cb);

    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = *sem;
    wait_semaphore_info.value = sem->value() + 1;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait_semaphore_info;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;
    vkQueueSubmit2(*cq, 1, &submit, *fence);
    task = task_monitor_->Add(fence, {cb, sem, position, quats, scales, cov3d, colors, sh, opacity});
  }

  // Graphics queue: make visible
  {
    auto cb = gq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    VkBufferMemoryBarrier2 acquire_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barrier.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
    acquire_barrier.dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT;
    acquire_barrier.srcQueueFamilyIndex = tq->family_index();
    acquire_barrier.dstQueueFamilyIndex = gq->family_index();
    acquire_barrier.buffer = *index_buffer;
    acquire_barrier.offset = 0;
    acquire_barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo acquire_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    acquire_dependency_info.bufferMemoryBarrierCount = 1;
    acquire_dependency_info.pBufferMemoryBarriers = &acquire_barrier;
    vkCmdPipelineBarrier2(*cb, &acquire_dependency_info);

    vkEndCommandBuffer(*cb);

    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = *sem;
    wait_semaphore_info.value = sem->value() + 1;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait_semaphore_info;
    vkQueueSubmit2(*gq, 1, &submit, *fence);
    task_monitor_->Add(fence, {cb, sem, index_buffer});
  }

  sem->Increment();

  return std::make_shared<GaussianSplats>(size, sh_degree, position, cov3d, sh, opacity, index_buffer, task);
}

std::shared_ptr<GaussianSplats> Renderer::LoadFromPly(const std::string& path, int sh_degree) {
  std::ifstream in(path, std::ios::binary);

  // parse header
  std::unordered_map<std::string, int> offsets;
  int offset = 0;
  uint32_t point_count = 0;
  std::string line;
  while (std::getline(in, line)) {
    if (line == "end_header") break;

    std::istringstream iss(line);
    std::string word;
    iss >> word;
    if (word == "property") {
      int size = 0;
      std::string type, property;
      iss >> type >> property;
      if (type == "float") {
        size = 4;
      }
      offsets[property] = offset;
      offset += size;
    } else if (word == "element") {
      std::string type;
      size_t count;
      iss >> type >> count;
      if (type == "vertex") {
        point_count = count;
      }
    }
  }

  int K = 0;
  for (const auto& [key, _] : offsets) {
    if (key.find("f_rest_") != std::string::npos) {
      K = std::max(K, std::stoi(key.substr(7)));
    }
  }
  K = K + 1;

  int sh_degree_data = 0;  // [0, 1, 2, 3], sh degree
  int sh_packed_size = 0;  // [1, 3, 7, 12], storage dimension for packing with f16vec4.
  switch (K) {
    case 0:  // no f_rest
      sh_degree_data = 0;
      sh_packed_size = 1;
      break;
    case 9:  // f_rest_[0..9)
      sh_degree_data = 1;
      sh_packed_size = 3;
      break;
    case 24:  // f_rest_[0..24)
      sh_degree_data = 2;
      sh_packed_size = 7;
      break;
    case 45:  // f_rest_[0..45)
      sh_degree_data = 3;
      sh_packed_size = 12;
      break;
    default:
      throw std::runtime_error("Unsupported SH degree for having f_rest_[0.." + std::to_string(K - 1) + "]");
  }
  K /= 3;

  if (sh_degree == -1) sh_degree = sh_degree_data;
  if (sh_degree > sh_degree_data) {
    throw std::runtime_error("SH degree for drawing is greater than the maximum degree of the data");
  }

  std::vector<uint32_t> ply_offsets(60);
  ply_offsets[0] = offsets["x"] / 4;
  ply_offsets[1] = offsets["y"] / 4;
  ply_offsets[2] = offsets["z"] / 4;
  ply_offsets[3] = offsets["scale_0"] / 4;
  ply_offsets[4] = offsets["scale_1"] / 4;
  ply_offsets[5] = offsets["scale_2"] / 4;
  ply_offsets[6] = offsets["rot_1"] / 4;  // qx
  ply_offsets[7] = offsets["rot_2"] / 4;  // qy
  ply_offsets[8] = offsets["rot_3"] / 4;  // qz
  ply_offsets[9] = offsets["rot_0"] / 4;  // qw
  ply_offsets[10 + 0] = offsets["f_dc_0"] / 4;
  ply_offsets[10 + 16] = offsets["f_dc_1"] / 4;
  ply_offsets[10 + 32] = offsets["f_dc_2"] / 4;
  for (int i = 0; i < K; ++i) {
    ply_offsets[10 + 1 + i] = offsets["f_rest_" + std::to_string(K * 0 + i)] / 4;
    ply_offsets[10 + 17 + i] = offsets["f_rest_" + std::to_string(K * 1 + i)] / 4;
    ply_offsets[10 + 33 + i] = offsets["f_rest_" + std::to_string(K * 2 + i)] / 4;
  }
  ply_offsets[58] = offsets["opacity"] / 4;
  ply_offsets[59] = offset / 4;

  std::vector<char> buffer(offset * point_count);
  in.read(buffer.data(), buffer.size());

  std::vector<uint32_t> index_data;
  index_data.reserve(6 * point_count);
  for (int i = 0; i < point_count; ++i) {
    index_data.push_back(4 * i + 0);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 3);
  }

  ParsePushConstants parse_ply_push_constants = {};
  parse_ply_push_constants.point_count = point_count;
  parse_ply_push_constants.sh_degree = sh_degree;

  // allocate buffers
  auto buffer_size = buffer.size() + 60 * sizeof(uint32_t);
  auto ply_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       buffer_size, true);
  auto ply_buffer =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);

  auto position = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 3 * sizeof(float));
  auto cov3d = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 6 * sizeof(float));
  auto sh = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                point_count * sh_packed_size * 4 * sizeof(uint16_t));
  auto opacity = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(float));

  auto index_stage =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, point_count * 6 * sizeof(uint32_t), true);
  auto index_buffer = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                          point_count * 6 * sizeof(uint32_t));

  std::memcpy(ply_stage->data(), ply_offsets.data(), ply_offsets.size() * sizeof(uint32_t));
  std::memcpy(ply_stage->data<char>() + ply_offsets.size() * sizeof(uint32_t), buffer.data(), buffer.size());
  std::memcpy(index_stage->data(), index_data.data(), index_data.size() * sizeof(uint32_t));

  auto sem = device_->AllocateSemaphore();

  auto cq = device_->compute_queue();
  auto gq = device_->graphics_queue();
  auto tq = device_->transfer_queue();

  std::shared_ptr<gpu::Task> task;

  // Transfer queue: stage to buffers
  {
    auto cb = tq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    VkBufferCopy region = {0, 0, buffer_size};
    vkCmdCopyBuffer(*cb, *ply_stage, *ply_buffer, 1, &region);

    region = {0, 0, index_stage->size()};
    vkCmdCopyBuffer(*cb, *index_stage, *index_buffer, 1, &region);

    // Release barrier
    std::vector<VkBufferMemoryBarrier2> release_barriers(2);
    release_barriers[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barriers[0].srcQueueFamilyIndex = tq->family_index();
    release_barriers[0].dstQueueFamilyIndex = cq->family_index();
    release_barriers[0].buffer = *ply_buffer;
    release_barriers[0].offset = 0;
    release_barriers[0].size = VK_WHOLE_SIZE;
    release_barriers[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barriers[1].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barriers[1].srcQueueFamilyIndex = tq->family_index();
    release_barriers[1].dstQueueFamilyIndex = gq->family_index();
    release_barriers[1].buffer = *index_buffer;
    release_barriers[1].offset = 0;
    release_barriers[1].size = VK_WHOLE_SIZE;
    VkDependencyInfo release_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    release_dependency_info.bufferMemoryBarrierCount = release_barriers.size();
    release_dependency_info.pBufferMemoryBarriers = release_barriers.data();
    vkCmdPipelineBarrier2(*cb, &release_dependency_info);

    vkEndCommandBuffer(*cb);

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_info.semaphore = *sem;
    signal_semaphore_info.value = sem->value() + 1;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal_semaphore_info;

    vkQueueSubmit2(*tq, 1, &submit, *fence);
    task_monitor_->Add(fence, {cb, sem, ply_stage, ply_buffer, index_stage, index_buffer});
  }

  // Compute queue: parse ply
  {
    auto cb = cq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    // Acquire barrier
    VkBufferMemoryBarrier2 acquire_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    acquire_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    acquire_barrier.srcQueueFamilyIndex = tq->family_index();
    acquire_barrier.dstQueueFamilyIndex = cq->family_index();
    acquire_barrier.buffer = *ply_buffer;
    acquire_barrier.offset = 0;
    acquire_barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo acquire_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    acquire_dependency_info.bufferMemoryBarrierCount = 1;
    acquire_dependency_info.pBufferMemoryBarriers = &acquire_barrier;
    vkCmdPipelineBarrier2(*cb, &acquire_dependency_info);

    // ply_buffer -> gaussian_splats
    cmdPushDescriptorSet(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *parse_pipeline_layout_,
                         {*ply_buffer, *position, *cov3d, *opacity, *sh});
    vkCmdPushConstants(*cb, *parse_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(parse_ply_push_constants),
                       &parse_ply_push_constants);

    vkCmdBindPipeline(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *parse_ply_pipeline_);
    vkCmdDispatch(*cb, WorkgroupSize(point_count, 256), 1, 1);

    // Visibility barrier
    VkMemoryBarrier2 visibility_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    visibility_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    visibility_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    visibility_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    visibility_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    VkDependencyInfo visibility_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    visibility_dependency_info.memoryBarrierCount = 1;
    visibility_dependency_info.pMemoryBarriers = &visibility_barrier;
    vkCmdPipelineBarrier2(*cb, &visibility_dependency_info);

    vkEndCommandBuffer(*cb);

    // Submit
    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = *sem;
    wait_semaphore_info.value = sem->value() + 1;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait_semaphore_info;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;

    vkQueueSubmit2(*cq, 1, &submit, *fence);
    task = task_monitor_->Add(fence, {cb, sem, parse_ply_pipeline_, ply_buffer, position, cov3d, sh, opacity});
  }

  // Graphics queue: acquire index buffer
  {
    auto cb = gq->AllocateCommandBuffer();
    auto fence = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    // Acquire barrier
    VkBufferMemoryBarrier2 acquire_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barrier.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
    acquire_barrier.dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT;
    acquire_barrier.srcQueueFamilyIndex = tq->family_index();
    acquire_barrier.dstQueueFamilyIndex = gq->family_index();
    acquire_barrier.buffer = *index_buffer;
    acquire_barrier.offset = 0;
    acquire_barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo acquire_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    acquire_dependency_info.bufferMemoryBarrierCount = 1;
    acquire_dependency_info.pBufferMemoryBarriers = &acquire_barrier;
    vkCmdPipelineBarrier2(*cb, &acquire_dependency_info);

    vkEndCommandBuffer(*cb);

    // Submit
    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = *sem;
    wait_semaphore_info.value = sem->value() + 1;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait_semaphore_info;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;

    vkQueueSubmit2(*gq, 1, &submit, *fence);
    task_monitor_->Add(fence, {cb, sem, index_buffer});
  }
  sem->Increment();

  return std::make_shared<GaussianSplats>(point_count, sh_degree, position, cov3d, sh, opacity, index_buffer, task);
}

std::shared_ptr<RenderedImage> Renderer::Draw(std::shared_ptr<GaussianSplats> splats, const DrawOptions& draw_options,
                                              uint8_t* dst) {
  std::shared_ptr<RenderedImage> rendered_image;

  uint32_t width = draw_options.width;
  uint32_t height = draw_options.height;

  auto cq = device_->compute_queue();
  auto gq = device_->graphics_queue();
  auto tq = device_->transfer_queue();

  auto N = splats->size();
  auto position = splats->position();
  auto cov3d = splats->cov3d();
  auto sh = splats->sh();
  auto opacity = splats->opacity();
  auto index_buffer = splats->index_buffer();

  ComputePushConstants compute_push_constants;
  compute_push_constants.model = glm::mat4(1.f);
  compute_push_constants.point_count = N;
  compute_push_constants.eps2d = draw_options.eps2d;
  compute_push_constants.sh_degree_data = splats->sh_degree();
  compute_push_constants.sh_degree_draw = draw_options.sh_degree == -1 ? splats->sh_degree() : draw_options.sh_degree;

  GraphicsPushConstants graphics_push_constants;
  graphics_push_constants.background = glm::vec4(draw_options.background, 1.f);

  Camera camera_data;
  camera_data.projection = draw_options.projection;
  camera_data.view = draw_options.view;
  camera_data.camera_position = glm::inverse(draw_options.view)[3];
  camera_data.screen_size = glm::uvec2(width, height);

  // Update storages
  const auto& double_buffer = double_buffer_[frame_index_ % 2];
  auto compute_storage = double_buffer.compute_storage;
  auto graphics_storage = double_buffer.graphics_storage;
  auto transfer_storage = double_buffer.transfer_storage;
  auto csem = double_buffer.compute_semaphore;
  auto cval = csem->value();
  auto gsem = double_buffer.graphics_semaphore;
  auto gval = gsem->value();
  auto tsem = double_buffer.transfer_semaphore;
  auto tval = tsem->value();

  compute_storage->Update(N, sorter_->GetStorageRequirements(N));
  graphics_storage->Update(width, height);
  transfer_storage->Update(width, height);

  auto visible_point_count = compute_storage->visible_point_count();
  auto key = compute_storage->key();
  auto index = compute_storage->index();
  auto sort_storage = compute_storage->sort_storage();
  auto inverse_index = compute_storage->inverse_index();
  auto camera = compute_storage->camera();
  auto draw_indirect = compute_storage->draw_indirect();
  auto instances = compute_storage->instances();
  auto camera_stage = compute_storage->camera_stage();

  std::memcpy(camera_stage->data(), &camera_data, sizeof(Camera));

  // Compute queue
  {
    auto fence = device_->AllocateFence();
    auto cb = cq->AllocateCommandBuffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    VkBufferCopy region = {0, 0, sizeof(Camera)};
    vkCmdCopyBuffer(*cb, *camera_stage, *camera, 1, &region);
    vkCmdFillBuffer(*cb, *visible_point_count, 0, sizeof(uint32_t), 0);
    vkCmdFillBuffer(*cb, *inverse_index, 0, N * sizeof(uint32_t), -1);

    VkMemoryBarrier2 memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    // Rank
    cmdPushDescriptorSet(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *compute_pipeline_layout_,
                         {
                             *camera,
                             *position,
                             *visible_point_count,
                             *key,
                             *index,
                         });
    vkCmdPushConstants(*cb, *compute_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(compute_push_constants),
                       &compute_push_constants);
    vkCmdBindPipeline(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *rank_pipeline_);
    vkCmdDispatch(*cb, WorkgroupSize(N, 256), 1, 1);

    // Sort
    memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_TRANSFER_READ_BIT;
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    sorter_->SortKeyValueIndirect(*cb, N, *visible_point_count, *key, *index, *sort_storage);

    // Inverse index
    memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    cmdPushDescriptorSet(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *compute_pipeline_layout_,
                         {
                             *visible_point_count,
                             *index,
                             *inverse_index,
                         });
    vkCmdPushConstants(*cb, *compute_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(compute_push_constants),
                       &compute_push_constants);
    vkCmdBindPipeline(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *inverse_index_pipeline_);
    vkCmdDispatch(*cb, WorkgroupSize(N, 256), 1, 1);

    // Projection
    memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    cmdPushDescriptorSet(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *compute_pipeline_layout_,
                         {
                             *camera,
                             *position,
                             *cov3d,
                             *opacity,
                             *sh,
                             *visible_point_count,
                             *inverse_index,
                             *draw_indirect,
                             *instances,
                         });
    vkCmdBindPipeline(*cb, VK_PIPELINE_BIND_POINT_COMPUTE, *projection_pipeline_);
    vkCmdDispatch(*cb, WorkgroupSize(N, 256), 1, 1);

    // Release
    std::vector<VkBufferMemoryBarrier2> buffer_memory_barriers(2);
    buffer_memory_barriers[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    buffer_memory_barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    buffer_memory_barriers[0].srcQueueFamilyIndex = cq->family_index();
    buffer_memory_barriers[0].dstQueueFamilyIndex = gq->family_index();
    buffer_memory_barriers[0].buffer = *instances;
    buffer_memory_barriers[0].offset = 0;
    buffer_memory_barriers[0].size = VK_WHOLE_SIZE;
    buffer_memory_barriers[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    buffer_memory_barriers[1].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    buffer_memory_barriers[1].srcQueueFamilyIndex = cq->family_index();
    buffer_memory_barriers[1].dstQueueFamilyIndex = gq->family_index();
    buffer_memory_barriers[1].buffer = *draw_indirect;
    buffer_memory_barriers[1].offset = 0;
    buffer_memory_barriers[1].size = VK_WHOLE_SIZE;
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.bufferMemoryBarrierCount = buffer_memory_barriers.size();
    dependency_info.pBufferMemoryBarriers = buffer_memory_barriers.data();
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    vkEndCommandBuffer(*cb);

    // Submit
    std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos;
    if (frame_index_ >= 2) {
      // G[i-2].read before C[i].comp
      auto& wait_semaphore_info = wait_semaphore_infos.emplace_back();
      wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      wait_semaphore_info.semaphore = *gsem;
      wait_semaphore_info.value = gval - 2 + 1;
      wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    }

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_info.semaphore = *csem;
    signal_semaphore_info.value = cval + 1;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

    VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit_info.waitSemaphoreInfoCount = wait_semaphore_infos.size();
    submit_info.pWaitSemaphoreInfos = wait_semaphore_infos.data();
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_buffer_info;
    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;

    vkQueueSubmit2(*cq, 1, &submit_info, *fence);
    task_monitor_->Add(fence, {cb, csem, camera_stage, camera, position, cov3d, opacity, sh, visible_point_count, key,
                               index, sort_storage, inverse_index, draw_indirect, instances});
  }

  auto image = graphics_storage->image();
  auto image_u8 = graphics_storage->image_u8();

  // Graphics queue
  {
    auto fence = device_->AllocateFence();
    auto cb = gq->AllocateCommandBuffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    // Acquire
    std::vector<VkBufferMemoryBarrier2> buffer_memory_barriers(2);
    buffer_memory_barriers[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    buffer_memory_barriers[0].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    buffer_memory_barriers[0].srcQueueFamilyIndex = cq->family_index();
    buffer_memory_barriers[0].dstQueueFamilyIndex = gq->family_index();
    buffer_memory_barriers[0].buffer = *instances;
    buffer_memory_barriers[0].offset = 0;
    buffer_memory_barriers[0].size = VK_WHOLE_SIZE;
    buffer_memory_barriers[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    buffer_memory_barriers[1].dstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    buffer_memory_barriers[1].srcQueueFamilyIndex = cq->family_index();
    buffer_memory_barriers[1].dstQueueFamilyIndex = gq->family_index();
    buffer_memory_barriers[1].buffer = *draw_indirect;
    buffer_memory_barriers[1].offset = 0;
    buffer_memory_barriers[1].size = VK_WHOLE_SIZE;
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.bufferMemoryBarrierCount = buffer_memory_barriers.size();
    dependency_info.pBufferMemoryBarriers = buffer_memory_barriers.data();
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    // Layout transition to color attachment
    VkImageMemoryBarrier2 image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.image = *image;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    // Rendering
    VkRenderingAttachmentInfo color_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color_attachment.imageView = image->image_view();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color = {0.f, 0.f, 0.f, 0.f};
    VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering_info.renderArea.offset = {0, 0};
    rendering_info.renderArea.extent = {width, height};
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;
    vkCmdBeginRendering(*cb, &rendering_info);

    vkCmdPushConstants(*cb, *graphics_pipeline_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(graphics_push_constants), &graphics_push_constants);

    vkCmdBindPipeline(*cb, VK_PIPELINE_BIND_POINT_GRAPHICS, *splat_pipeline_);
    cmdPushDescriptorSet(*cb, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphics_pipeline_layout_, {*instances});

    VkViewport viewport = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
    vkCmdSetViewport(*cb, 0, 1, &viewport);
    VkRect2D scissor = {0, 0, width, height};
    vkCmdSetScissor(*cb, 0, 1, &scissor);

    vkCmdBindIndexBuffer(*cb, *index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexedIndirect(*cb, *draw_indirect, 0, 1, 0);

    vkCmdBindPipeline(*cb, VK_PIPELINE_BIND_POINT_GRAPHICS, *splat_background_pipeline_);
    vkCmdDraw(*cb, 3, 1, 0, 0);

    vkCmdEndRendering(*cb);

    // float -> uint8
    image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    image_memory_barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_memory_barrier.image = *image;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.image = *image_u8;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    VkImageBlit image_region = {};
    image_region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    image_region.srcOffsets[0] = {0, 0, 0};
    image_region.srcOffsets[1] = {static_cast<int>(width), static_cast<int>(height), 1};
    image_region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    image_region.dstOffsets[0] = {0, 0, 0};
    image_region.dstOffsets[1] = {static_cast<int>(width), static_cast<int>(height), 1};
    vkCmdBlitImage(*cb, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *image_u8, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &image_region, VK_FILTER_NEAREST);

    // Layout transition to transfer src, and release
    image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
    image_memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = gq->family_index();
    image_memory_barrier.dstQueueFamilyIndex = tq->family_index();
    image_memory_barrier.image = *image_u8;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    vkEndCommandBuffer(*cb);

    // Submit
    std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos(1);
    // C[i].comp before G[i].read
    wait_semaphore_infos[0] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_infos[0].semaphore = *csem;
    wait_semaphore_infos[0].value = cval + 1;
    wait_semaphore_infos[0].stageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                                        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

    if (frame_index_ >= 2) {
      // T[i-2].xfer before G[i].output
      auto& wait_semaphore_info = wait_semaphore_infos.emplace_back();
      wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      wait_semaphore_info.semaphore = *tsem;
      wait_semaphore_info.value = tval - 1 + 1;
      wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    std::vector<VkSemaphoreSubmitInfo> signal_semaphore_infos(2);
    // G[i].read
    signal_semaphore_infos[0] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_infos[0].semaphore = *gsem;
    signal_semaphore_infos[0].value = gval + 1;
    signal_semaphore_infos[0].stageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                                          VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

    // G[i].blit
    signal_semaphore_infos[1] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_infos[1].semaphore = *gsem;
    signal_semaphore_infos[1].value = gval + 2;
    signal_semaphore_infos[1].stageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

    VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit_info.waitSemaphoreInfoCount = wait_semaphore_infos.size();
    submit_info.pWaitSemaphoreInfos = wait_semaphore_infos.data();
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_buffer_info;
    submit_info.signalSemaphoreInfoCount = signal_semaphore_infos.size();
    submit_info.pSignalSemaphoreInfos = signal_semaphore_infos.data();

    vkQueueSubmit2(*gq, 1, &submit_info, *fence);
    task_monitor_->Add(fence, {cb, image, instances, index_buffer, draw_indirect, gsem});
  }

  auto image_buffer = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT, width * height * 4, true);
  {
    auto fence = device_->AllocateFence();
    auto cb = tq->AllocateCommandBuffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(*cb, &begin_info);

    // Acquire
    VkImageMemoryBarrier2 image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = gq->family_index();
    image_memory_barrier.dstQueueFamilyIndex = tq->family_index();
    image_memory_barrier.image = *image_u8;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(*cb, &dependency_info);

    // Image to buffer
    VkBufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(*cb, *image_u8, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *image_buffer, 1, &region);

    vkEndCommandBuffer(*cb);

    // Submit
    // G[i].blit before T[i].xfer
    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = *gsem;
    wait_semaphore_info.value = gval + 2;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = *cb;

    // T[i].xfer
    VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_info.semaphore = *tsem;
    signal_semaphore_info.value = tval + 1;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = &wait_semaphore_info;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_buffer_info;
    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;

    vkQueueSubmit2(*tq, 1, &submit_info, *fence);
    auto task = task_monitor_->Add(fence, {cb, image, image_buffer, tsem}, [width, height, image_buffer, dst] {
      std::memcpy(dst, image_buffer->data<uint8_t>(), width * height * 4);
    });

    rendered_image = std::make_shared<RenderedImage>(width, height, task);
  }

  csem->Increment();
  gsem->Increment();
  gsem->Increment();
  tsem->Increment();
  frame_index_++;

  return rendered_image;
}

}  // namespace core
}  // namespace vkgs
