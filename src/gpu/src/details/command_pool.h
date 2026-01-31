#ifndef VKGS_GPU_DETAILS_COMMAND_POOL_H
#define VKGS_GPU_DETAILS_COMMAND_POOL_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace gpu {

class Command;
class Device;

class CommandPoolImpl;
class CommandPool : public Handle<CommandPool, CommandPoolImpl> {
 public:
  static CommandPool Create(Device device, uint32_t queue_family_index);

  Command Allocate();
  void Free(VkCommandBuffer command_buffer);
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_COMMAND_POOL_H
