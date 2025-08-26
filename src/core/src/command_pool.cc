#include "command_pool.h"

#include "vkgs/core/module.h"

#include "command.h"

namespace vkgs {
namespace core {

CommandPool::CommandPool(std::shared_ptr<Module> module, uint32_t queue_family_index)
    : module_(module), queue_family_index_(queue_family_index) {
  VkCommandPoolCreateInfo command_pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  command_pool_info.queueFamilyIndex = queue_family_index;
  vkCreateCommandPool(module_->device(), &command_pool_info, NULL, &command_pool_);
}

CommandPool::~CommandPool() { vkDestroyCommandPool(module_->device(), command_pool_, NULL); }

std::shared_ptr<Command> CommandPool::Allocate() {
  VkCommandBuffer command_buffer;

  if (command_buffers_.empty()) {
    VkCommandBufferAllocateInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    command_buffer_info.commandPool = command_pool_;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(module_->device(), &command_buffer_info, &command_buffer);
  } else {
    command_buffer = command_buffers_.back();
    command_buffers_.pop_back();
  }

  auto command = std::make_shared<Command>(shared_from_this(), command_buffer);
  return command;
}

void CommandPool::Free(VkCommandBuffer command_buffer) { command_buffers_.push_back(command_buffer); }

}  // namespace core
}  // namespace vkgs
