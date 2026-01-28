#ifndef VKGS_GPU_DETAILS_COMMAND_H
#define VKGS_GPU_DETAILS_COMMAND_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace gpu {

class CommandPool;
class Device;

class CommandImpl;
class Command : public Handle<Command, CommandImpl> {
 public:
  static Command Create(Device device, CommandPool command_pool, VkCommandBuffer cb);

  operator VkCommandBuffer() const;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_COMMAND_H
