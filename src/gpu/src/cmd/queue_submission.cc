#include "vkgs/gpu/cmd/queue_submission.h"

namespace vkgs {
namespace gpu {
namespace cmd {

QueueSubmission::QueueSubmission() = default;

QueueSubmission::~QueueSubmission() = default;

QueueSubmission& QueueSubmission::Wait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  wait_semaphore_info.semaphore = semaphore;
  wait_semaphore_info.value = value;
  wait_semaphore_info.stageMask = stage;
  wait_semaphore_infos_.push_back(wait_semaphore_info);
  return *this;
}

QueueSubmission& QueueSubmission::Command(VkCommandBuffer cb) {
  VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  command_buffer_info.commandBuffer = cb;
  command_buffer_infos_.push_back(command_buffer_info);
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

VkResult QueueSubmission::Submit(VkQueue queue, VkFence fence) {
  VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submit.waitSemaphoreInfoCount = wait_semaphore_infos_.size();
  submit.pWaitSemaphoreInfos = wait_semaphore_infos_.data();
  submit.commandBufferInfoCount = command_buffer_infos_.size();
  submit.pCommandBufferInfos = command_buffer_infos_.data();
  submit.signalSemaphoreInfoCount = signal_semaphore_infos_.size();
  submit.pSignalSemaphoreInfos = signal_semaphore_infos_.data();

  auto result = vkQueueSubmit2(queue, 1, &submit, fence);

  wait_semaphore_infos_.clear();
  command_buffer_infos_.clear();
  signal_semaphore_infos_.clear();

  return result;
}

}  // namespace cmd
}  // namespace gpu
}  // namespace vkgs
