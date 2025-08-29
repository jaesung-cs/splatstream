#ifndef VKGS_CORE_QUEUE_H
#define VKGS_CORE_QUEUE_H

#include "volk.h"

#include "command_pool.h"

namespace vkgs {
namespace core {

class Module;

class Queue {
 public:
  Queue(Module* module, VkQueue queue, uint32_t family_index);
  ~Queue() = default;

  auto queue() const noexcept { return queue_; }
  auto family_index() const noexcept { return family_index_; }

  std::shared_ptr<Command> AllocateCommandBuffer();

 private:
  Module* module_;

  VkQueue queue_ = VK_NULL_HANDLE;
  uint32_t family_index_ = 0;

  std::shared_ptr<CommandPool> command_pool_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_QUEUE_H
