#include "sorter.h"

#include "vkgs/core/module.h"

namespace vkgs {
namespace core {

Sorter::Sorter(VkDevice device, VkPhysicalDevice physical_device, VmaAllocator allocator)
    : device_(device), physical_device_(physical_device), allocator_(allocator) {
  VrdxSorterCreateInfo sorter_info = {};
  sorter_info.physicalDevice = physical_device;
  sorter_info.device = device;
  vrdxCreateSorter(&sorter_info, &sorter_);
}

Sorter::~Sorter() {
  if (storage_) {
    vmaDestroyBuffer(allocator_, storage_, allocation_);
  }

  vrdxDestroySorter(sorter_);
}

void Sorter::Sort(VkCommandBuffer cb, VkBuffer buffer, size_t size) {
  auto element_count = size / sizeof(uint32_t);

  VrdxSorterStorageRequirements requirements;
  vrdxGetSorterStorageRequirements(sorter_, element_count, &requirements);

  if (requirements.size > storage_size_) {
    if (storage_) {
      vmaDestroyBuffer(allocator_, storage_, allocation_);
    }

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = requirements.size;
    buffer_info.usage = requirements.usage;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer(allocator_, &buffer_info, &allocation_info, &storage_, &allocation_, NULL);

    storage_size_ = requirements.size;
  }

  vrdxCmdSort(cb, sorter_, element_count, buffer, 0, storage_, 0, VK_NULL_HANDLE, 0);
}

}  // namespace core
}  // namespace vkgs
