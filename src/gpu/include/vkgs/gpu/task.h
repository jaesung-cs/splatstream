#ifndef VKGS_GPU_TASK_H
#define VKGS_GPU_TASK_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "export_api.h"
#include "device.h"

namespace vkgs {
namespace gpu {

class Object;
class Queue;
class Fence;
class Command;

class VKGS_GPU_API Task {
 protected:
  enum class QueueType {
    TRANSFER,
    COMPUTE,
    GRAPHICS,
  };

 public:
  Task(QueueType queue_type);

  virtual ~Task();

  auto device() const noexcept { return device_; }

  VkCommandBuffer command_buffer() const;
  auto fence() const noexcept { return fence_; }

  Task& Keep(std::shared_ptr<Object> object);

  Task& Wait(VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Task& Wait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Task& WaitIf(bool condition, VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Task& WaitIf(bool condition, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Task& Signal(VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Task& Signal(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Task& PostCallback(std::function<void()> callback);

  std::shared_ptr<QueueTask> Submit();

 private:
  std::shared_ptr<Device> device_;
  std::shared_ptr<Queue> queue_;
  std::shared_ptr<Fence> fence_;
  std::shared_ptr<Command> command_;
  std::vector<std::shared_ptr<Object>> objects_;
  std::function<void()> callback_;

  std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos_;
  std::vector<VkSemaphoreSubmitInfo> signal_semaphore_infos_;

  bool submitted_ = false;
};

class VKGS_GPU_API ComputeTask : public Task {
 public:
  ComputeTask() : Task(QueueType::COMPUTE) {}
  ~ComputeTask() override = default;
};

class VKGS_GPU_API GraphicsTask : public Task {
 public:
  GraphicsTask() : Task(QueueType::GRAPHICS) {}
  ~GraphicsTask() override = default;
};

class VKGS_GPU_API TransferTask : public Task {
 public:
  TransferTask() : Task(QueueType::TRANSFER) {}
  ~TransferTask() override = default;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_H
