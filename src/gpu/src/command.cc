#include "command.h"

#include "command_pool.h"

namespace vkgs {
namespace gpu {

Command::Command(std::shared_ptr<CommandPool> command_pool, VkCommandBuffer cb)
    : command_pool_(command_pool), cb_(cb) {}

Command::~Command() { command_pool_->Free(cb_); }

}  // namespace gpu
}  // namespace vkgs
