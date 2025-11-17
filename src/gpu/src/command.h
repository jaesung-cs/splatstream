#ifndef VKGS_GPU_COMMAND_H
#define VKGS_GPU_COMMAND_H

#include <memory>

#include <vulkan/vulkan.h>

namespace vkgs {
namespace gpu {

class CommandPool;

class Command {
 public:
  Command(std::shared_ptr<CommandPool> command_pool, VkCommandBuffer cb);
  ~Command();

  operator VkCommandBuffer() const noexcept { return cb_; }

 private:
  std::shared_ptr<CommandPool> command_pool_;
  VkCommandBuffer cb_ = VK_NULL_HANDLE;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_COMMAND_H
