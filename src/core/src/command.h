#ifndef VKGS_CORE_COMMAND_H
#define VKGS_CORE_COMMAND_H

#include <memory>

#include "volk.h"

namespace vkgs {
namespace core {

class Module;

class Command {
 public:
  Command(std::shared_ptr<Module> module, VkCommandBuffer cb);
  ~Command();

 private:
  std::shared_ptr<Module> module_;
  VkCommandBuffer cb_ = VK_NULL_HANDLE;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_COMMAND_H
