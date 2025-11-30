#ifndef VKGS_GPU_BUFFER_H
#define VKGS_GPU_BUFFER_H

#include <memory>

#include <vulkan/vulkan.h>

#include "export_api.h"
#include "object.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API BufferImpl : public Object {
 public:
  BufferImpl(VkBufferUsageFlags usage, VkDeviceSize size, bool host = false);
  ~BufferImpl() override;

  operator VkBuffer() const noexcept { return buffer_; }

  VkDeviceSize size() const noexcept { return size_; }
  void* data() noexcept { return ptr_; }
  const void* data() const noexcept { return ptr_; }

  template <typename T>
  T* data() noexcept {
    return static_cast<T*>(ptr_);
  }

  template <typename T>
  const T* data() const noexcept {
    return static_cast<const T*>(ptr_);
  }

 private:
  VkDeviceSize size_ = 0;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  void* allocation_ = VK_NULL_HANDLE;
  void* ptr_ = nullptr;
};

class VKGS_GPU_API Buffer : public SharedAccessor<Buffer, BufferImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_BUFFER_H
