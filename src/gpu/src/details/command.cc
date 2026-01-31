#include "details/command.h"

#include "vkgs/gpu/device.h"

#include "details/command_pool.h"

namespace vkgs {
namespace gpu {

class CommandImpl {
 public:
  void __init__(Device device, CommandPool command_pool, VkCommandBuffer cb) {
    device_ = device;
    command_pool_ = command_pool;
    cb_ = cb;
  }

  void __del__() { command_pool_.Free(cb_); }

  operator VkCommandBuffer() const noexcept { return cb_; }

 private:
  Device device_;
  CommandPool command_pool_;
  VkCommandBuffer cb_ = VK_NULL_HANDLE;
};

Command Command::Create(Device device, CommandPool command_pool, VkCommandBuffer cb) {
  return Make<CommandImpl>(device, command_pool, cb);
}

Command::operator VkCommandBuffer() const { return *impl_; }

}  // namespace gpu
}  // namespace vkgs
