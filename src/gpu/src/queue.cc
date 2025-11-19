#include "vkgs/gpu/queue.h"

#include "command_pool.h"

namespace vkgs {
namespace gpu {

Queue::Queue(VkDevice device, VkQueue queue, uint32_t family_index)
    : device_(device), queue_(queue), family_index_(family_index) {
  command_pool_ = std::make_shared<CommandPool>(device_, family_index_);
}

Queue::~Queue() = default;

std::shared_ptr<Command> Queue::AllocateCommandBuffer() { return command_pool_->Allocate(); }

}  // namespace gpu
}  // namespace vkgs
