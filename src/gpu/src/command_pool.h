#ifndef VKGS_GPU_COMMAND_POOL_H
#define VKGS_GPU_COMMAND_POOL_H

#include <memory>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace gpu {

class Command;

class CommandPool : public std::enable_shared_from_this<CommandPool> {
 public:
  explicit CommandPool(VkDevice device, uint32_t queue_family_index);
  ~CommandPool();

  VkCommandPool command_pool() const noexcept { return command_pool_; }
  uint32_t queue_family_index() const noexcept { return queue_family_index_; }

  std::shared_ptr<Command> Allocate();
  void Free(VkCommandBuffer command_buffer);

 private:
  VkDevice device_;
  uint32_t queue_family_index_;

  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> command_buffers_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_COMMAND_POOL_H
