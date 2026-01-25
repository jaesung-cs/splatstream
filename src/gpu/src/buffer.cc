#include "vkgs/gpu/buffer.h"

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class BufferImpl : public Object {
 public:
  void __init__(VkBufferUsageFlags usage, VkDeviceSize size, bool host) {
    size_ = size;

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocator allocator = static_cast<VmaAllocator>(device_.allocator());
    VmaAllocation allocation = VK_NULL_HANDLE;
    if (host) {
      VmaAllocationCreateInfo allocation_info = {};
      allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
      allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
      VmaAllocationInfo map_info;
      vmaCreateBuffer(allocator, &buffer_info, &allocation_info, &buffer_, &allocation, &map_info);
      ptr_ = map_info.pMappedData;
    } else {
      VmaAllocationCreateInfo allocation_info = {};
      allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
      vmaCreateBuffer(allocator, &buffer_info, &allocation_info, &buffer_, &allocation, NULL);
    }
    allocation_ = allocation;
  }

  void __del__() {
    VmaAllocator allocator = static_cast<VmaAllocator>(device_.allocator());
    VmaAllocation allocation = static_cast<VmaAllocation>(allocation_);

    vmaDestroyBuffer(allocator, buffer_, allocation);
  }

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

Buffer Buffer::Create(VkBufferUsageFlags usage, VkDeviceSize size, bool host) {
  return Make<BufferImpl>(usage, size, host);
}

Buffer::operator VkBuffer() const { return *impl_; }

void Buffer::Keep() const { impl_->Keep(); }

VkDeviceSize Buffer::size() const noexcept { return impl_->size(); }
void* Buffer::data() noexcept { return impl_->data(); }
const void* Buffer::data() const noexcept { return impl_->data(); }

}  // namespace gpu
}  // namespace vkgs
