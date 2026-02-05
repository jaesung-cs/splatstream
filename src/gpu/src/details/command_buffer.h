#ifndef VKGS_GPU_DETAILS_COMMAND_BUFFER_H
#define VKGS_GPU_DETAILS_COMMAND_BUFFER_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace gpu {

class CommandPool;
class Device;

class CommandBufferImpl;
class CommandBuffer : public Handle<CommandBuffer, CommandBufferImpl> {
 public:
  static CommandBuffer Create(Device device, CommandPool command_pool, VkCommandBuffer cb);

  operator VkCommandBuffer() const;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_COMMAND_H
