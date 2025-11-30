#include "vkgs/gpu/queue.h"

#include "vkgs/gpu/command_pool.h"

#include "vkgs/gpu/command.h"

namespace vkgs {
namespace gpu {

QueueImpl::QueueImpl(VkDevice device, VkQueue queue, uint32_t family_index)
    : device_(device), queue_(queue), family_index_(family_index) {
  command_pool_ = CommandPool::Create(device_, family_index_);
}

QueueImpl::~QueueImpl() = default;

Command QueueImpl::AllocateCommandBuffer() { return command_pool_->Allocate(); }

}  // namespace gpu
}  // namespace vkgs
