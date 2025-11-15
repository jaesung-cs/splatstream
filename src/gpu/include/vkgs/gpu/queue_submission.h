#ifndef VKGS_GPU_QUEUE_SUBMISSION_H
#define VKGS_GPU_QUEUE_SUBMISSION_H

#include <cstdint>
#include <vector>
#include <memory>
#include <functional>

#include <vulkan/vulkan.h>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Device;
class Queue;
class Task;

class VKGS_GPU_API QueueSubmission {
 public:
  QueueSubmission(std::shared_ptr<Device> device, std::shared_ptr<Queue> queue,
                  std::function<void(VkCommandBuffer)> task_callback, std::function<void()> host_callback);
  ~QueueSubmission();

  QueueSubmission& Wait(VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  QueueSubmission& Wait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  QueueSubmission& WaitIf(bool condition, VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  QueueSubmission& WaitIf(bool condition, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  QueueSubmission& Signal(VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  QueueSubmission& Signal(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  std::shared_ptr<Task> Submit();

 private:
  std::shared_ptr<Device> device_;
  std::shared_ptr<Queue> queue_;
  std::function<void(VkCommandBuffer)> task_callback_;
  std::function<void()> host_callback_;

  std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos_;
  std::vector<VkSemaphoreSubmitInfo> signal_semaphore_infos_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_QUEUE_SUBMISSION_H
