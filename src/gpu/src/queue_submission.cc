#include "vkgs/gpu/queue_submission.h"

#include <volk.h>

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/fence.h"
#include "vkgs/gpu/task.h"
#include "vkgs/gpu/queue.h"

#include "command.h"

namespace vkgs {
namespace gpu {

QueueSubmission::QueueSubmission(std::shared_ptr<Device> device, std::shared_ptr<Queue> queue,
                                 std::function<void(VkCommandBuffer)> task_callback,
                                 std::function<void()> host_callback)
    : device_(device), queue_(queue), task_callback_(task_callback), host_callback_(host_callback) {}

QueueSubmission::~QueueSubmission() = default;

QueueSubmission& QueueSubmission::Wait(VkSemaphore semaphore, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  wait_semaphore_info.semaphore = semaphore;
  wait_semaphore_info.stageMask = stage;
  wait_semaphore_infos_.push_back(wait_semaphore_info);
  return *this;
}

QueueSubmission& QueueSubmission::Wait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  wait_semaphore_info.semaphore = semaphore;
  wait_semaphore_info.value = value;
  wait_semaphore_info.stageMask = stage;
  wait_semaphore_infos_.push_back(wait_semaphore_info);
  return *this;
}

QueueSubmission& QueueSubmission::WaitIf(bool condition, VkSemaphore semaphore, VkPipelineStageFlags2 stage) {
  if (condition) return Wait(semaphore, stage);
  return *this;
}

QueueSubmission& QueueSubmission::WaitIf(bool condition, VkSemaphore semaphore, uint64_t value,
                                         VkPipelineStageFlags2 stage) {
  if (condition) return Wait(semaphore, value, stage);
  return *this;
}

QueueSubmission& QueueSubmission::Signal(VkSemaphore semaphore, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = semaphore;
  signal_semaphore_info.stageMask = stage;
  signal_semaphore_infos_.push_back(signal_semaphore_info);
  return *this;
}

QueueSubmission& QueueSubmission::Signal(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = semaphore;
  signal_semaphore_info.value = value;
  signal_semaphore_info.stageMask = stage;
  signal_semaphore_infos_.push_back(signal_semaphore_info);
  return *this;
}

std::shared_ptr<Task> QueueSubmission::Submit() {
  auto fence = device_->AllocateFence();
  auto cb = queue_->AllocateCommandBuffer();

  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(*cb, &begin_info);

  task_callback_(*cb);

  vkEndCommandBuffer(*cb);

  VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  command_buffer_info.commandBuffer = *cb;

  VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submit.waitSemaphoreInfoCount = wait_semaphore_infos_.size();
  submit.pWaitSemaphoreInfos = wait_semaphore_infos_.data();
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos = &command_buffer_info;
  submit.signalSemaphoreInfoCount = signal_semaphore_infos_.size();
  submit.pSignalSemaphoreInfos = signal_semaphore_infos_.data();
  vkQueueSubmit2(*queue_, 1, &submit, *fence);

  wait_semaphore_infos_.clear();
  signal_semaphore_infos_.clear();

  auto task = device_->AddTask(fence, cb, std::move(task_callback_), std::move(host_callback_));
  return task;
}

}  // namespace gpu
}  // namespace vkgs
