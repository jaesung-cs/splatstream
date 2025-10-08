#include "buffer.h"

namespace vkgs {
namespace core {

Buffer::Buffer(VkDevice device, VmaAllocator allocator, VkBufferUsageFlags usage, VkDeviceSize size)
    : device_(device), allocator_(allocator), size_(size) {
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = size;
  buffer_info.usage = usage;

  VmaAllocationCreateInfo allocation_info = {};
  allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
  vmaCreateBuffer(allocator_, &buffer_info, &allocation_info, &buffer_, &allocation_, NULL);
}

Buffer::~Buffer() { vmaDestroyBuffer(allocator_, buffer_, allocation_); }

}  // namespace core
}  // namespace vkgs
