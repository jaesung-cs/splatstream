#ifndef VKGS_GPU_COMMAND_H
#define VKGS_GPU_COMMAND_H

#include <memory>
#include <functional>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class QueueTask;

enum class QueueType {
  TRANSFER,
  COMPUTE,
  GRAPHICS,
};

class CommandImpl;
class Command {
 public:
  Command(QueueType queue_type);
  ~Command();

  operator VkCommandBuffer() const;

  Command& Keep(AnyHandle object);

  Command& Wait(VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Command& Wait(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Command& WaitIf(bool condition, VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Command& WaitIf(bool condition, VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Command& Signal(VkSemaphore semaphore, VkPipelineStageFlags2 stage);
  Command& Signal(VkSemaphore semaphore, uint64_t value, VkPipelineStageFlags2 stage);

  Command& PostCallback(std::function<void()> callback);

  QueueTask Submit();

 private:
  std::unique_ptr<CommandImpl> impl_;
};

class VKGS_GPU_API ComputeCommand : public Command {
 public:
  ComputeCommand() : Command(QueueType::COMPUTE) {}
};

class VKGS_GPU_API GraphicsCommand : public Command {
 public:
  GraphicsCommand() : Command(QueueType::GRAPHICS) {}
};

class VKGS_GPU_API TransferCommand : public Command {
 public:
  TransferCommand() : Command(QueueType::TRANSFER) {}
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_COMMAND_H
