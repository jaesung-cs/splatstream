#include "sorter.h"

#include "vkgs/core/module.h"
#include "vkgs/core/buffer.h"

namespace vkgs {
namespace core {

Sorter::Sorter(Module* module) : module_(module) {
  VrdxSorterCreateInfo sorter_info = {};
  sorter_info.physicalDevice = module->physical_device();
  sorter_info.device = module->device();
  vrdxCreateSorter(&sorter_info, &sorter_);
}

Sorter::~Sorter() {
  if (storage_) {
    vmaDestroyBuffer(module_->allocator(), storage_, allocation_);
  }

  vrdxDestroySorter(sorter_);
}

void Sorter::Sort(VkCommandBuffer cb, std::shared_ptr<Buffer> buffer) {
  auto element_count = buffer->size() / sizeof(uint32_t);

  VrdxSorterStorageRequirements requirements;
  vrdxGetSorterStorageRequirements(sorter_, element_count, &requirements);

  if (requirements.size > storage_size_) {
    if (storage_) {
      vmaDestroyBuffer(module_->allocator(), storage_, allocation_);
    }

    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = requirements.size;
    buffer_info.usage = requirements.usage;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer(module_->allocator(), &buffer_info, &allocation_info, &storage_, &allocation_, NULL);

    storage_size_ = requirements.size;
  }

  vrdxCmdSort(cb, sorter_, element_count, buffer->buffer(), 0, storage_, 0, VK_NULL_HANDLE, 0);
}

}  // namespace core
}  // namespace vkgs
