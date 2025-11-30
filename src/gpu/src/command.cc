#include "vkgs/gpu/command.h"

#include "vkgs/gpu/command_pool.h"

namespace vkgs {
namespace gpu {

CommandImpl::CommandImpl(CommandPool command_pool, VkCommandBuffer cb) : command_pool_(command_pool), cb_(cb) {}

CommandImpl::~CommandImpl() { command_pool_->Free(cb_); }

}  // namespace gpu
}  // namespace vkgs
