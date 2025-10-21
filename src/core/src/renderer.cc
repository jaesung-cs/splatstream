#include "vkgs/core/renderer.h"

#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
#include "generated/rank.h"
#include "generated/inverse_index.h"
#include "generated/projection.h"
#include "generated/splat_vert.h"
#include "generated/splat_frag.h"
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

  parse_ply_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_,
                                  {
                                      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                  },
                                  {{VK_SHADER_STAGE_COMPUTE_BIT, 0, 4}});

  parse_ply_pipeline_ = gpu::ComputePipeline::Create(*device_, *parse_ply_pipeline_layout_, parse_ply);

  rank_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_,
                                  {
                                      {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                      {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                  },
                                  {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants)}});

  rank_pipeline_ = gpu::ComputePipeline::Create(*device_, *rank_pipeline_layout_, rank);

  inverse_index_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_, {
                                                {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                                {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                                {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                            });

  inverse_index_pipeline_ = gpu::ComputePipeline::Create(*device_, *inverse_index_pipeline_layout_, inverse_index);

  projection_pipeline_layout_ =
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
                                  {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants)}});

  projection_pipeline_ = gpu::ComputePipeline::Create(*device_, *projection_pipeline_layout_, projection);

  splat_pipeline_layout_ =
      gpu::PipelineLayout::Create(*device_, {{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}});

  splat_pipeline_ = gpu::GraphicsPipeline::Create(*device_, *splat_pipeline_layout_, splat_vert, splat_frag,
                                                  VK_FORMAT_R32G32B32A32_SFLOAT);
}

Renderer::~Renderer() = default;

const std::string& Renderer::device_name() const noexcept { return device_->device_name(); }
uint32_t Renderer::graphics_queue_index() const noexcept { return device_->graphics_queue_index(); }
uint32_t Renderer::compute_queue_index() const noexcept { return device_->compute_queue_index(); }
uint32_t Renderer::transfer_queue_index() const noexcept { return device_->transfer_queue_index(); }

std::shared_ptr<GaussianSplats> Renderer::load_from_ply(const std::string& path) {
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
  for (int i = 0; i < 15; ++i) {
    ply_offsets[10 + 1 + i] = offsets["f_rest_" + std::to_string(i)] / 4;
    ply_offsets[10 + 17 + i] = offsets["f_rest_" + std::to_string(15 + i)] / 4;
    ply_offsets[10 + 33 + i] = offsets["f_rest_" + std::to_string(30 + i)] / 4;
  }
  ply_offsets[58] = offsets["opacity"] / 4;
  ply_offsets[59] = offset / 4;

  std::vector<char> buffer(offset * point_count);
  in.read(buffer.data(), buffer.size());

  // allocate buffers
  auto buffer_size = buffer.size() + 60 * sizeof(uint32_t);
  auto ply_stage = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       buffer_size, true);
  auto ply_buffer =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);

  auto position = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 3 * sizeof(float));
  auto cov3d = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 6 * sizeof(float));
  auto sh = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 48 * 2);
  auto opacity = gpu::Buffer::Create(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(float));

  std::memcpy(ply_stage->data(), ply_offsets.data(), ply_offsets.size() * sizeof(uint32_t));
  std::memcpy(ply_stage->data<char>() + ply_offsets.size() * sizeof(uint32_t), buffer.data(), buffer.size());

  auto sem = device_->AllocateSemaphore();
  {
    auto command0 = device_->transfer_queue()->AllocateCommandBuffer();
    auto cb0 = command0->command_buffer();
    auto fence0 = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb0, &begin_info);

    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = buffer_size;
    vkCmdCopyBuffer(cb0, *ply_stage, *ply_buffer, 1, &region);

    // Release barrier
    VkBufferMemoryBarrier2 release_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barrier.srcQueueFamilyIndex = device_->transfer_queue()->family_index();
    release_barrier.dstQueueFamilyIndex = device_->compute_queue()->family_index();
    release_barrier.buffer = *ply_buffer;
    release_barrier.offset = 0;
    release_barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo release_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    release_dependency_info.bufferMemoryBarrierCount = 1;
    release_dependency_info.pBufferMemoryBarriers = &release_barrier;
    vkCmdPipelineBarrier2(cb0, &release_dependency_info);

    vkEndCommandBuffer(cb0);

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = cb0;

    VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_info.semaphore = *sem;
    signal_semaphore_info.value = sem->value() + 1;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal_semaphore_info;

    vkQueueSubmit2(device_->transfer_queue()->queue(), 1, &submit, fence0->fence());
    task_monitor_->Add(fence0, {command0, sem, ply_stage, ply_buffer});
  }

  {
    auto command1 = device_->compute_queue()->AllocateCommandBuffer();
    auto cb1 = command1->command_buffer();
    auto fence1 = device_->AllocateFence();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb1, &begin_info);

    // Acquire barrier
    VkBufferMemoryBarrier2 acquire_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    acquire_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    acquire_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    acquire_barrier.srcQueueFamilyIndex = device_->transfer_queue()->family_index();
    acquire_barrier.dstQueueFamilyIndex = device_->compute_queue()->family_index();
    acquire_barrier.buffer = *ply_buffer;
    acquire_barrier.offset = 0;
    acquire_barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo acquire_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    acquire_dependency_info.bufferMemoryBarrierCount = 1;
    acquire_dependency_info.pBufferMemoryBarriers = &acquire_barrier;
    vkCmdPipelineBarrier2(cb1, &acquire_dependency_info);

    // ply_buffer -> gaussian_splats
    vkCmdPushConstants(cb1, *parse_ply_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(point_count),
                       &point_count);
    cmdPushDescriptorSet(cb1, VK_PIPELINE_BIND_POINT_COMPUTE, *parse_ply_pipeline_layout_,
                         {*ply_buffer, *position, *cov3d, *opacity, *sh});

    vkCmdBindPipeline(cb1, VK_PIPELINE_BIND_POINT_COMPUTE, *parse_ply_pipeline_);
    vkCmdDispatch(cb1, WorkgroupSize(point_count, 256), 1, 1);

    // Visibility barrier
    VkMemoryBarrier2 visibility_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    visibility_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    visibility_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    visibility_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    visibility_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    VkDependencyInfo visibility_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    visibility_dependency_info.memoryBarrierCount = 1;
    visibility_dependency_info.pMemoryBarriers = &visibility_barrier;
    vkCmdPipelineBarrier2(cb1, &visibility_dependency_info);

    vkEndCommandBuffer(cb1);

    // Acquire
    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = *sem;
    wait_semaphore_info.value = sem->value() + 1;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = cb1;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait_semaphore_info;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;

    vkQueueSubmit2(device_->compute_queue()->queue(), 1, &submit, fence1->fence());
    task_monitor_->Add(fence1, {command1, sem, parse_ply_pipeline_, ply_buffer, position, cov3d, sh, opacity});
  }

  sem->Increment();

  return std::make_shared<GaussianSplats>(point_count, position, cov3d, sh, opacity);
}

std::shared_ptr<RenderedImage> Renderer::draw(std::shared_ptr<GaussianSplats> splats, const glm::mat4& view,
                                              const glm::mat4& projection, uint32_t width, uint32_t height,
                                              float* dst) {
  std::shared_ptr<RenderedImage> rendered_image;

  auto N = splats->size();

  PushConstants push_constants;
  push_constants.model = glm::mat4(1.f);
  push_constants.point_count = N;

  Camera camera_data;
  camera_data.projection = projection;
  camera_data.view = view;
  camera_data.camera_position = glm::inverse(view)[3];
  camera_data.screen_size = glm::uvec2(width, height);

  std::vector<uint32_t> index_data;
  index_data.reserve(6 * N);
  for (int i = 0; i < N; ++i) {
    index_data.push_back(4 * i + 0);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 2);
    index_data.push_back(4 * i + 1);
    index_data.push_back(4 * i + 3);
  }

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
  auto index_buffer = compute_storage->index_buffer();
  auto camera_stage = compute_storage->camera_stage();
  auto index_stage = compute_storage->index_stage();

  std::memcpy(index_stage->data(), index_data.data(), index_data.size() * sizeof(uint32_t));
  std::memcpy(camera_stage->data(), &camera_data, sizeof(Camera));

  // Compute queue
  {
    auto fence = device_->AllocateFence();
    auto command = device_->compute_queue()->AllocateCommandBuffer();
    auto cb = command->command_buffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &begin_info);

    VkBufferCopy region = {0, 0, sizeof(Camera)};
    vkCmdCopyBuffer(cb, *camera_stage, *camera, 1, &region);

    region = {0, 0, index_data.size() * sizeof(uint32_t)};
    vkCmdCopyBuffer(cb, *index_stage, *index_buffer, 1, &region);

    vkCmdFillBuffer(cb, *visible_point_count, 0, sizeof(uint32_t), 0);
    vkCmdFillBuffer(cb, *inverse_index, 0, N * sizeof(uint32_t), -1);

    VkMemoryBarrier2 memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    // Rank
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, *rank_pipeline_);
    vkCmdPushConstants(cb, *rank_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants),
                       &push_constants);
    cmdPushDescriptorSet(cb, VK_PIPELINE_BIND_POINT_COMPUTE, *rank_pipeline_layout_,
                         {
                             *camera,
                             *splats->position(),
                             *visible_point_count,
                             *key,
                             *index,
                         });
    vkCmdDispatch(cb, WorkgroupSize(N, 256), 1, 1);

    // Sort
    memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_TRANSFER_READ_BIT;
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    sorter_->SortKeyValueIndirect(cb, N, *visible_point_count, *key, *index, *sort_storage);

    // Inverse index
    memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, *inverse_index_pipeline_);
    cmdPushDescriptorSet(cb, VK_PIPELINE_BIND_POINT_COMPUTE, *inverse_index_pipeline_layout_,
                         {
                             *visible_point_count,
                             *index,
                             *inverse_index,
                         });
    vkCmdDispatch(cb, WorkgroupSize(N, 256), 1, 1);

    // Projection
    memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.memoryBarrierCount = 1;
    dependency_info.pMemoryBarriers = &memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, *projection_pipeline_);
    vkCmdPushConstants(cb, *rank_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push_constants),
                       &push_constants);
    cmdPushDescriptorSet(cb, VK_PIPELINE_BIND_POINT_COMPUTE, *projection_pipeline_layout_,
                         {
                             *camera,
                             *splats->position(),
                             *splats->cov3d(),
                             *splats->opacity(),
                             *splats->sh(),
                             *visible_point_count,
                             *inverse_index,
                             *draw_indirect,
                             *instances,
                         });
    vkCmdDispatch(cb, WorkgroupSize(N, 256), 1, 1);

    // Release
    std::vector<VkBufferMemoryBarrier2> buffer_memory_barriers(3);
    buffer_memory_barriers[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    buffer_memory_barriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    buffer_memory_barriers[0].srcQueueFamilyIndex = device_->compute_queue()->family_index();
    buffer_memory_barriers[0].dstQueueFamilyIndex = device_->graphics_queue()->family_index();
    buffer_memory_barriers[0].buffer = *index_buffer;
    buffer_memory_barriers[0].offset = 0;
    buffer_memory_barriers[0].size = VK_WHOLE_SIZE;
    buffer_memory_barriers[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    buffer_memory_barriers[1].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    buffer_memory_barriers[1].srcQueueFamilyIndex = device_->compute_queue()->family_index();
    buffer_memory_barriers[1].dstQueueFamilyIndex = device_->graphics_queue()->family_index();
    buffer_memory_barriers[1].buffer = *instances;
    buffer_memory_barriers[1].offset = 0;
    buffer_memory_barriers[1].size = VK_WHOLE_SIZE;
    buffer_memory_barriers[2] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[2].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    buffer_memory_barriers[2].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    buffer_memory_barriers[2].srcQueueFamilyIndex = device_->compute_queue()->family_index();
    buffer_memory_barriers[2].dstQueueFamilyIndex = device_->graphics_queue()->family_index();
    buffer_memory_barriers[2].buffer = *draw_indirect;
    buffer_memory_barriers[2].offset = 0;
    buffer_memory_barriers[2].size = VK_WHOLE_SIZE;
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.bufferMemoryBarrierCount = buffer_memory_barriers.size();
    dependency_info.pBufferMemoryBarriers = buffer_memory_barriers.data();
    vkCmdPipelineBarrier2(cb, &dependency_info);

    vkEndCommandBuffer(cb);

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
    command_buffer_info.commandBuffer = cb;

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

    vkQueueSubmit2(device_->compute_queue()->queue(), 1, &submit_info, fence->fence());
    task_monitor_->Add(fence, {command, csem, camera_stage, index_stage, camera, visible_point_count, key, index,
                               sort_storage, inverse_index, index_buffer, draw_indirect, instances});
  }

  auto image = graphics_storage->image();

  // Graphics queue
  {
    auto fence = device_->AllocateFence();
    auto command = device_->graphics_queue()->AllocateCommandBuffer();
    auto cb = command->command_buffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &begin_info);

    // Acquire
    std::vector<VkBufferMemoryBarrier2> buffer_memory_barriers(3);
    buffer_memory_barriers[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
    buffer_memory_barriers[0].dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT;
    buffer_memory_barriers[0].srcQueueFamilyIndex = device_->compute_queue()->family_index();
    buffer_memory_barriers[0].dstQueueFamilyIndex = device_->graphics_queue()->family_index();
    buffer_memory_barriers[0].buffer = *index_buffer;
    buffer_memory_barriers[0].offset = 0;
    buffer_memory_barriers[0].size = VK_WHOLE_SIZE;
    buffer_memory_barriers[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    buffer_memory_barriers[1].dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    buffer_memory_barriers[1].srcQueueFamilyIndex = device_->compute_queue()->family_index();
    buffer_memory_barriers[1].dstQueueFamilyIndex = device_->graphics_queue()->family_index();
    buffer_memory_barriers[1].buffer = *instances;
    buffer_memory_barriers[1].offset = 0;
    buffer_memory_barriers[1].size = VK_WHOLE_SIZE;
    buffer_memory_barriers[2] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    buffer_memory_barriers[2].dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    buffer_memory_barriers[2].dstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    buffer_memory_barriers[2].srcQueueFamilyIndex = device_->compute_queue()->family_index();
    buffer_memory_barriers[2].dstQueueFamilyIndex = device_->graphics_queue()->family_index();
    buffer_memory_barriers[2].buffer = *draw_indirect;
    buffer_memory_barriers[2].offset = 0;
    buffer_memory_barriers[2].size = VK_WHOLE_SIZE;
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.bufferMemoryBarrierCount = buffer_memory_barriers.size();
    dependency_info.pBufferMemoryBarriers = buffer_memory_barriers.data();
    vkCmdPipelineBarrier2(cb, &dependency_info);

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
    vkCmdPipelineBarrier2(cb, &dependency_info);

    // Rendering
    VkRenderingAttachmentInfo color_attachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color_attachment.imageView = image->image_view();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color = {0.f, 0.f, 0.f, 1.f};
    VkRenderingInfo rendering_info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering_info.renderArea.offset = {0, 0};
    rendering_info.renderArea.extent = {width, height};
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;
    vkCmdBeginRendering(cb, &rendering_info);

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, *splat_pipeline_);
    cmdPushDescriptorSet(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, *splat_pipeline_layout_, {*instances});

    VkViewport viewport = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f};
    vkCmdSetViewport(cb, 0, 1, &viewport);
    VkRect2D scissor = {0, 0, width, height};
    vkCmdSetScissor(cb, 0, 1, &scissor);

    vkCmdBindIndexBuffer(cb, *index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexedIndirect(cb, *draw_indirect, 0, 1, 0);

    vkCmdEndRendering(cb);

    // Layout transition to transfer src, and release
    image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    image_memory_barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = device_->graphics_queue()->family_index();
    image_memory_barrier.dstQueueFamilyIndex = device_->transfer_queue()->family_index();
    image_memory_barrier.image = *image;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    vkEndCommandBuffer(cb);

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
    command_buffer_info.commandBuffer = cb;

    std::vector<VkSemaphoreSubmitInfo> signal_semaphore_infos(2);
    // G[i].read
    signal_semaphore_infos[0] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_infos[0].semaphore = *gsem;
    signal_semaphore_infos[0].value = gval + 1;
    signal_semaphore_infos[0].stageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
                                          VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

    // G[i].output
    signal_semaphore_infos[1] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal_semaphore_infos[1].semaphore = *gsem;
    signal_semaphore_infos[1].value = gval + 2;
    signal_semaphore_infos[1].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit_info.waitSemaphoreInfoCount = wait_semaphore_infos.size();
    submit_info.pWaitSemaphoreInfos = wait_semaphore_infos.data();
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_buffer_info;
    submit_info.signalSemaphoreInfoCount = signal_semaphore_infos.size();
    submit_info.pSignalSemaphoreInfos = signal_semaphore_infos.data();

    vkQueueSubmit2(device_->graphics_queue()->queue(), 1, &submit_info, fence->fence());
    task_monitor_->Add(fence, {command, image, instances, index_buffer, draw_indirect, gsem});
  }

  auto image_buffer =
      gpu::Buffer::Create(device_, VK_BUFFER_USAGE_TRANSFER_DST_BIT, width * height * 4 * sizeof(float), true);
  {
    auto fence = device_->AllocateFence();
    auto command = device_->transfer_queue()->AllocateCommandBuffer();
    auto cb = command->command_buffer();

    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &begin_info);

    // Acquire
    VkImageMemoryBarrier2 image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_memory_barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = device_->graphics_queue()->family_index();
    image_memory_barrier.dstQueueFamilyIndex = device_->transfer_queue()->family_index();
    image_memory_barrier.image = *image;
    image_memory_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_memory_barrier;
    vkCmdPipelineBarrier2(cb, &dependency_info);

    // Image to buffer
    VkBufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(cb, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *image_buffer, 1, &region);

    vkEndCommandBuffer(cb);

    // Submit
    // G[i].output before T[i].xfer
    VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    wait_semaphore_info.semaphore = *gsem;
    wait_semaphore_info.value = gval + 2;
    wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    command_buffer_info.commandBuffer = cb;

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

    vkQueueSubmit2(device_->transfer_queue()->queue(), 1, &submit_info, fence->fence());
    auto task = task_monitor_->Add(fence, {command, image, image_buffer, tsem}, [width, height, image_buffer, dst] {
      std::memcpy(dst, image_buffer->data<float>(), width * height * 4 * sizeof(float));
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
