#include "details/command_pool.h"

#include <vector>

#include "volk.h"

#include "vkgs/gpu/device.h"

#include "details/command.h"

namespace vkgs {
namespace gpu {

class CommandPoolImpl : public EnableHandleFromThis<CommandPool, CommandPoolImpl> {
 public:
  void __init__(Device device, uint32_t queue_family_index) {
    device_ = device;
    queue_family_index_ = queue_family_index;

    VkCommandPoolCreateInfo command_pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_index;
    vkCreateCommandPool(device_, &command_pool_info, NULL, &command_pool_);
  }

  void __del__() { vkDestroyCommandPool(device_, command_pool_, NULL); }

  Command Allocate() {
    VkCommandBuffer command_buffer;

    if (command_buffers_.empty()) {
      VkCommandBufferAllocateInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
      command_buffer_info.commandPool = command_pool_;
      command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      command_buffer_info.commandBufferCount = 1;
      vkAllocateCommandBuffers(device_, &command_buffer_info, &command_buffer);
    } else {
      command_buffer = command_buffers_.back();
      command_buffers_.pop_back();
    }

    return Command::Create(device_.lock(), HandleFromThis(), command_buffer);
  }

  void Free(VkCommandBuffer command_buffer) { command_buffers_.push_back(command_buffer); }

 private:
  Device::Weak device_;
  uint32_t queue_family_index_;

  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> command_buffers_;
};

CommandPool CommandPool::Create(Device device, uint32_t queue_family_index) {
  return Make<CommandPoolImpl>(device, queue_family_index);
}

Command CommandPool::Allocate() { return impl_->Allocate(); }
void CommandPool::Free(VkCommandBuffer command_buffer) { impl_->Free(command_buffer); }

}  // namespace gpu
}  // namespace vkgs
