#ifndef VKGS_GPU_DETAILS_COMMAND_POOL_H
#define VKGS_GPU_DETAILS_COMMAND_POOL_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"

namespace vkgs {
namespace gpu {

class Command;

class CommandPoolImpl : public std::enable_shared_from_this<CommandPoolImpl> {
 public:
  explicit CommandPoolImpl(VkDevice device, uint32_t queue_family_index);
  ~CommandPoolImpl();

  Command Allocate();
  void Free(VkCommandBuffer command_buffer);

 private:
  VkDevice device_;
  uint32_t queue_family_index_;

  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> command_buffers_;
};

class CommandPool : public SharedAccessor<CommandPool, CommandPoolImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_COMMAND_POOL_H
