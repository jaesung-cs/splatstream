#include "vkgs/core/module.h"

#include <fstream>
#include <unordered_map>
#include <sstream>

#include "vkgs/core/gaussian_splats.h"

#include "device.h"
#include "sorter.h"
#include "buffer.h"
#include "semaphore.h"
#include "fence.h"
#include "queue.h"
#include "command.h"
#include "task_monitor.h"

namespace vkgs {
namespace core {

Module::Module() {
  device_ = std::make_shared<Device>();
  task_monitor_ = std::make_shared<TaskMonitor>();
  sorter_ = std::make_shared<Sorter>(device_->device(), device_->physical_device(), device_->allocator());
}

const std::string& Module::device_name() const noexcept { return device_->device_name(); }
uint32_t Module::graphics_queue_index() const noexcept { return device_->graphics_queue_index(); }
uint32_t Module::compute_queue_index() const noexcept { return device_->compute_queue_index(); }
uint32_t Module::transfer_queue_index() const noexcept { return device_->transfer_queue_index(); }

std::shared_ptr<GaussianSplats> Module::load_from_ply(const std::string& path) {
  std::ifstream in(path, std::ios::binary);

  // parse header
  std::unordered_map<std::string, int> offsets;
  int offset = 0;
  size_t point_count = 0;
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

  // TODO: allocate buffers
  size_t buffer_size = 0;
  std::shared_ptr<Buffer> ply_stage;
  std::shared_ptr<Buffer> ply_buffer;

  std::shared_ptr<Buffer> position;
  std::shared_ptr<Buffer> cov3d;
  std::shared_ptr<Buffer> sh;
  std::shared_ptr<Buffer> opacity;

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
    vkCmdCopyBuffer(cb0, ply_stage->buffer(), ply_buffer->buffer(), 1, &region);

    // Release barrier
    VkBufferMemoryBarrier2 release_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
    release_barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    release_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    release_barrier.srcQueueFamilyIndex = device_->transfer_queue()->family_index();
    release_barrier.dstQueueFamilyIndex = device_->compute_queue()->family_index();
    release_barrier.buffer = ply_buffer->buffer();
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
    auto command1 = device_->graphics_queue()->AllocateCommandBuffer();
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
    acquire_barrier.buffer = ply_buffer->buffer();
    acquire_barrier.offset = 0;
    acquire_barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo acquire_dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    acquire_dependency_info.bufferMemoryBarrierCount = 1;
    acquire_dependency_info.pBufferMemoryBarriers = &acquire_barrier;
    vkCmdPipelineBarrier2(cb1, &acquire_dependency_info);

    // TODO: ply_buffer -> gaussian_splats
    /*
    vkCmdBindPipeline(cb1, VK_PIPELINE_BIND_POINT_COMPUTE, parse_ply_pipeline_);
    vkCmdBindDescriptorSets(cb1, VK_PIPELINE_BIND_POINT_COMPUTE, parse_ply_pipeline_layout_, 0, 1,
                            &parse_ply_descriptor_set_, 0, nullptr);
    vkCmdDispatch(cb1, point_count, 1, 1);
    */

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
