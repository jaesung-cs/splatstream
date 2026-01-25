#ifndef VKGS_GPU_QUEUE_H
#define VKGS_GPU_QUEUE_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class Command;

class QueueImpl;
class VKGS_GPU_API Queue : public Handle<Queue, QueueImpl> {
 public:
  static Queue Create(VkDevice device, VkQueue queue, uint32_t family_index);

  operator VkQueue() const;
  operator uint32_t() const;
  auto family_index() const;

  Command AllocateCommandBuffer();
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_QUEUE_H
