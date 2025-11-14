#ifndef VKGS_GPU_BUFFER_H
#define VKGS_GPU_BUFFER_H

#include "object.h"

#include <memory>

#include <vulkan/vulkan.h>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Device;

class VKGS_GPU_API Buffer : public Object {
 public:
  static std::shared_ptr<Buffer> Create(std::shared_ptr<Device> device, VkBufferUsageFlags usage, VkDeviceSize size,
                                        bool host = false);

 public:
  Buffer(std::shared_ptr<Device> device, VkBufferUsageFlags usage, VkDeviceSize size, bool host = false);
  ~Buffer() override;

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
  std::shared_ptr<Device> device_;

  VkDeviceSize size_ = 0;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  void* allocation_ = VK_NULL_HANDLE;
  void* ptr_ = nullptr;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_BUFFER_H
