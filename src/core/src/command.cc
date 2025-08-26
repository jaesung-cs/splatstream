#include "command.h"

namespace vkgs {
namespace core {

Command::Command(std::shared_ptr<Module> module, VkCommandBuffer cb) : module_(module), cb_(cb) {}

Command::~Command() {}

}  // namespace core
}  // namespace vkgs
