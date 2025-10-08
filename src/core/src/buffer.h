#ifndef VKGS_CORE_BUFFER_H
#define VKGS_CORE_BUFFER_H

#include "volk.h"
#include "vk_mem_alloc.h"

namespace vkgs {
namespace core {

class Buffer {
 public:
  Buffer(VkDevice device, VmaAllocator allocator, VkBufferUsageFlags usage, VkDeviceSize size);
  ~Buffer();

  VkBuffer buffer() const noexcept { return buffer_; }
  VkDeviceSize size() const noexcept { return size_; }

 private:
  VkDevice device_;
  VmaAllocator allocator_;

  VkDeviceSize size_ = 0;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_BUFFER_H
