#include "vkgs/gpu/host_buffer.h"

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class HostBufferImpl : public Object {
 public:
  void __init__(VkBufferUsageFlags usage, VkDeviceSize size) {
    size_ = size;

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    VmaAllocationInfo map_info;
    vmaCreateBuffer(device_.allocator(), &buffer_info, &allocation_info, &buffer_, &allocation_, &map_info);
    ptr_ = map_info.pMappedData;
  }

  void __del__() { vmaDestroyBuffer(device_.allocator(), buffer_, allocation_); }

  operator VkBuffer() const noexcept { return buffer_; }

  VkDeviceSize size() const noexcept { return size_; }
  void* data() noexcept { return ptr_; }
  const void* data() const noexcept { return ptr_; }

 private:
  VkDeviceSize size_ = 0;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
  void* ptr_ = nullptr;
};

HostBuffer HostBuffer::Create(VkBufferUsageFlags usage, VkDeviceSize size) { return Make<HostBufferImpl>(usage, size); }

HostBuffer::operator VkBuffer() const { return *impl_; }

void HostBuffer::Keep() const { impl_->Keep(); }

VkDeviceSize HostBuffer::size() const noexcept { return impl_->size(); }
void* HostBuffer::data() noexcept { return impl_->data(); }
const void* HostBuffer::data() const noexcept { return impl_->data(); }

}  // namespace gpu
}  // namespace vkgs
