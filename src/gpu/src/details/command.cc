#include "vkgs/gpu/details/command.h"

#include "vkgs/gpu/details/command_pool.h"

namespace vkgs {
namespace gpu {

class CommandImpl {
 public:
  void __init__(CommandPool command_pool, VkCommandBuffer cb) {
    command_pool_ = command_pool;
    cb_ = cb;
  }

  void __del__() { command_pool_.Free(cb_); }

  operator VkCommandBuffer() const noexcept { return cb_; }

 private:
  CommandPool command_pool_;
  VkCommandBuffer cb_ = VK_NULL_HANDLE;
};

Command Command::Create(CommandPool command_pool, VkCommandBuffer cb) { return Make<CommandImpl>(command_pool, cb); }

Command::operator VkCommandBuffer() const { return *impl_; }

}  // namespace gpu
}  // namespace vkgs
