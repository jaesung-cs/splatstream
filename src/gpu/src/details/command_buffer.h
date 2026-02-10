#ifndef VKGS_GPU_DETAILS_COMMAND_BUFFER_H
#define VKGS_GPU_DETAILS_COMMAND_BUFFER_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

#include "vkgs/gpu/queue_type.h"

namespace vkgs {
namespace gpu {

class CommandPool;
class Device;

class CommandBufferImpl;
class CommandBuffer : public Handle<CommandBuffer, CommandBufferImpl> {
 public:
  static CommandBuffer Create(QueueType queue_type);

  operator VkCommandBuffer() const;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_COMMAND_H
