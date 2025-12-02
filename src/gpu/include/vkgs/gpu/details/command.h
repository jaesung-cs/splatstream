#ifndef VKGS_GPU_DETAILS_COMMAND_H
#define VKGS_GPU_DETAILS_COMMAND_H

#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/gpu/details/command_pool.h"

namespace vkgs {
namespace gpu {

class CommandImpl {
 public:
  CommandImpl(CommandPool command_pool, VkCommandBuffer cb);
  ~CommandImpl();

  operator VkCommandBuffer() const noexcept { return cb_; }

 private:
  CommandPool command_pool_;
  VkCommandBuffer cb_ = VK_NULL_HANDLE;
};

class Command : public SharedAccessor<Command, CommandImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_COMMAND_H
