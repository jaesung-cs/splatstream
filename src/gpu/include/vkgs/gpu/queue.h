#ifndef VKGS_GPU_QUEUE_H
#define VKGS_GPU_QUEUE_H

#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Command;
class CommandPool;

class VKGS_GPU_API QueueImpl {
 public:
  QueueImpl(VkDevice device, VkQueue queue, uint32_t family_index);
  ~QueueImpl();

  operator VkQueue() const noexcept { return queue_; }
  operator uint32_t() const noexcept { return family_index_; }
  auto family_index() const noexcept { return family_index_; }

  std::shared_ptr<Command> AllocateCommandBuffer();

 private:
  VkDevice device_;

  VkQueue queue_ = VK_NULL_HANDLE;
  uint32_t family_index_ = 0;

  std::shared_ptr<CommandPool> command_pool_;
};

class VKGS_GPU_API Queue : public SharedAccessor<Queue, QueueImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_QUEUE_H
