#include "queue.h"

namespace vkgs {
namespace core {

Queue::Queue(Module* module, VkQueue queue, uint32_t family_index)
    : module_(module), queue_(queue), family_index_(family_index) {
  command_pool_ = std::make_shared<CommandPool>(module_, family_index_);
}

std::shared_ptr<Command> Queue::AllocateCommandBuffer() { return command_pool_->Allocate(); }

}  // namespace core
}  // namespace vkgs
