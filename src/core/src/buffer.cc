#include "buffer.h"

namespace vkgs {
namespace core {

std::shared_ptr<Buffer> Buffer::Create(VkDevice device, VmaAllocator allocator, VkBufferUsageFlags usage,
                                       VkDeviceSize size, bool host) {
  return std::make_shared<Buffer>(device, allocator, usage, size, host);
}

Buffer::Buffer(VkDevice device, VmaAllocator allocator, VkBufferUsageFlags usage, VkDeviceSize size, bool host)
    : device_(device), allocator_(allocator), size_(size) {
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = size;
  buffer_info.usage = usage;

  if (host) {
    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationInfo map_info;
    vmaCreateBuffer(allocator_, &buffer_info, &allocation_info, &buffer_, &allocation_, &map_info);
    ptr_ = map_info.pMappedData;
  } else {
    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer(allocator_, &buffer_info, &allocation_info, &buffer_, &allocation_, NULL);
  }
}

Buffer::~Buffer() { vmaDestroyBuffer(allocator_, buffer_, allocation_); }

}  // namespace core
}  // namespace vkgs
