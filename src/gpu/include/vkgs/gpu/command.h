#ifndef VKGS_GPU_COMMAND_H
#define VKGS_GPU_COMMAND_H

#include "object.h"

#include <memory>

#include "volk.h"

#include "export_api.h"

namespace vkgs {
namespace gpu {

class CommandPool;

class VKGS_GPU_API Command : public Object {
 public:
  Command(std::shared_ptr<CommandPool> command_pool, VkCommandBuffer cb);
  ~Command() override;

  VkCommandBuffer command_buffer() const noexcept { return cb_; }

 private:
  std::shared_ptr<CommandPool> command_pool_;
  VkCommandBuffer cb_ = VK_NULL_HANDLE;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_COMMAND_H
