#ifndef VKGS_CORE_COMMAND_H
#define VKGS_CORE_COMMAND_H

#include <memory>

#include "volk.h"

#include "object.h"

namespace vkgs {
namespace core {

class CommandPool;

class Command : public Object {
 public:
  Command(std::shared_ptr<CommandPool> command_pool, VkCommandBuffer cb);
  ~Command() override;

  VkCommandBuffer command_buffer() const noexcept { return cb_; }

 private:
  std::shared_ptr<CommandPool> command_pool_;
  VkCommandBuffer cb_ = VK_NULL_HANDLE;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_COMMAND_H
