#ifndef VKGS_GPU_TASK_H
#define VKGS_GPU_TASK_H

#include <memory>
#include <functional>

#include <vulkan/vulkan.h>

#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class Object;
class QueueTask;

enum class QueueType {
  TRANSFER,
  COMPUTE,
  GRAPHICS,
};

class TaskImpl;
class VKGS_GPU_API Task {
 public:
  Task(QueueType queue_type);

  auto device() const noexcept;

  VkCommandBuffer command_buffer() const;
  auto fence() const noexcept;

  Task& Keep(std::shared_ptr<Object> object);

  Task& Wait(VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Task& Wait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Task& WaitIf(bool condition, VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Task& WaitIf(bool condition, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Task& Signal(VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Task& Signal(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Task& PostCallback(std::function<void()> callback);

  QueueTask Submit();

 private:
  std::shared_ptr<TaskImpl> impl_;
};

class VKGS_GPU_API ComputeTask : public Task {
 public:
  ComputeTask() : Task(QueueType::COMPUTE) {}
};

class VKGS_GPU_API GraphicsTask : public Task {
 public:
  GraphicsTask() : Task(QueueType::GRAPHICS) {}
};

class VKGS_GPU_API TransferTask : public Task {
 public:
  TransferTask() : Task(QueueType::TRANSFER) {}
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_H
