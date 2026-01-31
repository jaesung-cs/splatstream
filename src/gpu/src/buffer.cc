#include "vkgs/gpu/buffer.h"

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class BufferImpl : public Object {
 public:
  void __init__(VkBufferUsageFlags usage, VkDeviceSize size) {
    size_ = size;

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer(device_.allocator(), &buffer_info, &allocation_info, &buffer_, &allocation_, NULL);
  }

  void __del__() { vmaDestroyBuffer(device_.allocator(), buffer_, allocation_); }

  operator VkBuffer() const noexcept { return buffer_; }

  VkDeviceSize size() const noexcept { return size_; }

 private:
  VkDeviceSize size_ = 0;
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;
};

Buffer Buffer::Create(VkBufferUsageFlags usage, VkDeviceSize size) { return Make<BufferImpl>(usage, size); }

Buffer::operator VkBuffer() const { return *impl_; }

void Buffer::Keep() const { impl_->Keep(); }

VkDeviceSize Buffer::size() const noexcept { return impl_->size(); }

}  // namespace gpu
}  // namespace vkgs
