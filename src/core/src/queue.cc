#include "queue.h"

#include "vkgs/core/module.h"

#include "semaphore_pool.h"

namespace vkgs {
namespace core {

Queue::Queue(Module* module, VkQueue queue, uint32_t family_index)
    : module_(module), queue_(queue), family_index_(family_index), semaphore_(module_->semaphore_pool()->Allocate()) {
  command_pool_ = std::make_shared<CommandPool>(module_, family_index_);
}

std::shared_ptr<Command> Queue::AllocateCommandBuffer() { return command_pool_->Allocate(); }

}  // namespace core
}  // namespace vkgs
