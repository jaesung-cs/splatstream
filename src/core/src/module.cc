#include "vkgs/core/module.h"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>

#include "vkgs/core/gaussian_splats.h"

#include "generated/parse_ply.h"
#include "buffer.h"
#include "device.h"
#include "sorter.h"
#include "buffer.h"
#include "semaphore.h"
#include "fence.h"
#include "queue.h"
#include "command.h"
#include "task_monitor.h"
#include "pipeline_layout.h"
#include "compute_pipeline.h"
#include "vulkan/vulkan_core.h"

namespace {

auto WorkgroupSize(size_t count, uint32_t local_size) { return (count + local_size - 1) / local_size; }

void cmdPushDescriptorSet(VkCommandBuffer cb, VkPipelineLayout pipeline_layout, uint32_t set,
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
  vkCmdPushDescriptorSet(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, set, writes.size(), writes.data());
}

}  // namespace

namespace vkgs {
namespace core {

Module::Module() {
  device_ = std::make_shared<Device>();
  task_monitor_ = std::make_shared<TaskMonitor>();
  sorter_ = std::make_shared<Sorter>(*device_, device_->physical_device(), device_->allocator());

  parse_ply_pipeline_layout_ =
      PipelineLayout::Create(*device_,
                             {
                                 {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                 {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                 {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                 {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                                 {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
                             },
                             {{VK_SHADER_STAGE_COMPUTE_BIT, 0, 4}});

  parse_ply_pipeline_ = ComputePipeline::Create(*device_, *parse_ply_pipeline_layout_, parse_ply);
}

Module::~Module() { device_->WaitIdle(); }

const std::string& Module::device_name() const noexcept { return device_->device_name(); }
uint32_t Module::graphics_queue_index() const noexcept { return device_->graphics_queue_index(); }
uint32_t Module::compute_queue_index() const noexcept { return device_->compute_queue_index(); }
uint32_t Module::transfer_queue_index() const noexcept { return device_->transfer_queue_index(); }

std::shared_ptr<GaussianSplats> Module::load_from_ply(const std::string& path) {
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

  std::cout << "offset: " << offset << std::endl;
  std::cout << "point_count: " << point_count << std::endl;

  std::vector<char> buffer(offset * point_count);
  in.read(buffer.data(), buffer.size());

  // allocate buffers
  auto buffer_size = buffer.size() + 60 * sizeof(uint32_t);
  auto ply_stage =
      Buffer::Create(*device_, device_->allocator(),
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, buffer_size, true);
  auto ply_buffer = Buffer::Create(*device_, device_->allocator(),
                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);

  auto position = Buffer::Create(*device_, device_->allocator(),
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 point_count * 3 * sizeof(float));
  auto cov3d = Buffer::Create(*device_, device_->allocator(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              point_count * 6 * sizeof(float));
  auto sh = Buffer::Create(*device_, device_->allocator(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * 48 * 2);
  auto opacity =
      Buffer::Create(*device_, device_->allocator(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_count * sizeof(float));

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
    signal_semaphore_info.semaphore = sem->semaphore();
    signal_semaphore_info.value = sem->value() + 1;
    signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &command_buffer_info;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal_semaphore_info;

    vkQueueSubmit2(device_->transfer_queue()->queue(), 1, &submit, fence0->fence());
    task_monitor_->Add(command0, sem, fence0, {ply_stage, ply_buffer});
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
    cmdPushDescriptorSet(cb1, *parse_ply_pipeline_layout_, 0, {*ply_buffer, *position, *cov3d, *opacity, *sh});

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
    wait_semaphore_info.semaphore = sem->semaphore();
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
    task_monitor_->Add(command1, sem, fence1, {ply_buffer, position, cov3d, sh, opacity});
  }

  sem->Increment();

  return std::make_shared<GaussianSplats>(point_count, position, cov3d, sh, opacity);
}

}  // namespace core
}  // namespace vkgs
