#ifndef VKGS_GPU_BUFFER_H
#define VKGS_GPU_BUFFER_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class BufferImpl;
class VKGS_GPU_API Buffer : public Handle<Buffer, BufferImpl> {
 public:
  static Buffer Create(VkBufferUsageFlags usage, VkDeviceSize size);

  operator VkBuffer() const;

  void Keep() const;

  VkDeviceSize size() const noexcept;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_BUFFER_H
