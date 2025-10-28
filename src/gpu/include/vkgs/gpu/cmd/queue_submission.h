#ifndef VKGS_GPU_CMD_QUEUE_SUBMISSION_H
#define VKGS_GPU_CMD_QUEUE_SUBMISSION_H

#include <cstdint>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace gpu {
namespace cmd {

class QueueSubmission {
 public:
  QueueSubmission();
  ~QueueSubmission();

  QueueSubmission& Wait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  QueueSubmission& Command(VkCommandBuffer cb);

  QueueSubmission& Signal(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  VkResult Submit(VkQueue queue, VkFence fence);

 private:
  std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos_;
  std::vector<VkCommandBufferSubmitInfo> command_buffer_infos_;
  std::vector<VkSemaphoreSubmitInfo> signal_semaphore_infos_;
};

}  // namespace cmd
}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_CMD_QUEUE_SUBMISSION_H
