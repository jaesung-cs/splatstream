#include "vkgs/gpu/buffer.h"

#include <volk.h>
#include <vk_mem_alloc.h>

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {

std::shared_ptr<Buffer> Buffer::Create(std::shared_ptr<Device> device, VkBufferUsageFlags usage, VkDeviceSize size,
                                       bool host) {
  return std::make_shared<Buffer>(device, usage, size, host);
}

Buffer::Buffer(std::shared_ptr<Device> device, VkBufferUsageFlags usage, VkDeviceSize size, bool host)
    : device_(device), size_(size) {
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = size;
  buffer_info.usage = usage;

  VmaAllocator allocator = static_cast<VmaAllocator>(device_->allocator());
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

Buffer::~Buffer() {
  VmaAllocator allocator = static_cast<VmaAllocator>(device_->allocator());
  VmaAllocation allocation = static_cast<VmaAllocation>(allocation_);

  vmaDestroyBuffer(allocator, buffer_, allocation);
}

}  // namespace gpu
}  // namespace vkgs
