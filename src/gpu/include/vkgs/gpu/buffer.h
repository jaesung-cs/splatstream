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
  static Buffer Create(VkBufferUsageFlags usage, VkDeviceSize size, bool host = false);

  operator VkBuffer() const;

  void Keep() const;

  VkDeviceSize size() const noexcept;
  void* data() noexcept;
  const void* data() const noexcept;

  template <typename T>
  T* data() noexcept {
    return static_cast<T*>(data());
  }

  template <typename T>
  const T* data() const noexcept {
    return static_cast<const T*>(data());
  }
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_BUFFER_H
