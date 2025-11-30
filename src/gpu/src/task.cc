#include "vkgs/gpu/task.h"

#include <volk.h>

#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/fence.h"
#include "vkgs/gpu/queue_task.h"

#include "command.h"

namespace vkgs {
namespace gpu {

Task::Task(QueueType queue_type) {
  device_ = GetDevice();
  device_->SetCurrentTask(this);

  switch (queue_type) {
    case QueueType::TRANSFER:
      queue_ = device_->transfer_queue();
      break;
    case QueueType::COMPUTE:
      queue_ = device_->compute_queue();
      break;
    case QueueType::GRAPHICS:
      queue_ = device_->graphics_queue();
      break;
  }

  command_ = queue_->AllocateCommandBuffer();
  fence_ = device_->AllocateFence();

  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(*command_, &begin_info);
}

Task::~Task() {
  if (!submitted_) {
    Submit();
  }
}

VkCommandBuffer Task::command_buffer() const { return *command_; }

Task& Task::Keep(std::shared_ptr<Object> object) {
  objects_.push_back(object);
  return *this;
}

Task& Task::Wait(VkSemaphore semaphore, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  wait_semaphore_info.semaphore = semaphore;
  wait_semaphore_info.stageMask = stage;
  wait_semaphore_infos_.push_back(wait_semaphore_info);
  return *this;
}

Task& Task::Wait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  wait_semaphore_info.semaphore = semaphore;
  wait_semaphore_info.value = value;
  wait_semaphore_info.stageMask = stage;
  wait_semaphore_infos_.push_back(wait_semaphore_info);
  return *this;
}

Task& Task::WaitIf(bool condition, VkSemaphore semaphore, VkPipelineStageFlags2 stage) {
  if (condition) return Wait(semaphore, stage);
  return *this;
}

Task& Task::WaitIf(bool condition, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage) {
  if (condition) return Wait(semaphore, value, stage);
  return *this;
}

Task& Task::Signal(VkSemaphore semaphore, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = semaphore;
  signal_semaphore_info.stageMask = stage;
  signal_semaphore_infos_.push_back(signal_semaphore_info);
  return *this;
}

Task& Task::Signal(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage) {
  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = semaphore;
  signal_semaphore_info.value = value;
  signal_semaphore_info.stageMask = stage;
  signal_semaphore_infos_.push_back(signal_semaphore_info);
  return *this;
}

Task& Task::PostCallback(std::function<void()> callback) {
  callback_ = callback;
  return *this;
}

QueueTask Task::Submit() {
  vkEndCommandBuffer(*command_);

  VkCommandBufferSubmitInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  command_buffer_info.commandBuffer = *command_;

  VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submit.waitSemaphoreInfoCount = wait_semaphore_infos_.size();
  submit.pWaitSemaphoreInfos = wait_semaphore_infos_.data();
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos = &command_buffer_info;
  submit.signalSemaphoreInfoCount = signal_semaphore_infos_.size();
  submit.pSignalSemaphoreInfos = signal_semaphore_infos_.data();
  vkQueueSubmit2(queue_, 1, &submit, fence_);

  auto task = device_->AddQueueTask(fence_, command_, std::move(objects_), callback_);
  submitted_ = true;
  device_->ClearCurrentTask();
  return task;
}

}  // namespace gpu
}  // namespace vkgs
