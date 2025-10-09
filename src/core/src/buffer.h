#ifndef VKGS_CORE_BUFFER_H
#define VKGS_CORE_BUFFER_H

#include <memory>

#include "volk.h"
#include "vk_mem_alloc.h"

namespace vkgs {
namespace core {

class Buffer {
 public:
  static std::shared_ptr<Buffer> Create(VkDevice device, VmaAllocator allocator, VkBufferUsageFlags usage,
                                        VkDeviceSize size, bool host = false);

 public:
  Buffer(VkDevice device, VmaAllocator allocator, VkBufferUsageFlags usage, VkDeviceSize size, bool host = false);
  ~Buffer();

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
  VkDevice device_;
  VmaAllocator allocator_;

  VkDeviceSize size_ = 0;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  void* ptr_ = nullptr;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_BUFFER_H
