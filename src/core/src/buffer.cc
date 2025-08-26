#include "vkgs/core/buffer.h"

#include "vkgs/core/module.h"

#include "semaphore.h"

namespace vkgs {
namespace core {

Buffer::Buffer(std::shared_ptr<Module> module, size_t size) : module_(module), size_(size) {
  auto allocator = module->allocator();
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = size;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  VmaAllocationCreateInfo allocation_info = {};
  allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
  vmaCreateBuffer(allocator, &buffer_info, &allocation_info, &buffer_, &allocation_, nullptr);

  // Stage buffer
  buffer_info.size = size;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
  allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
  VmaAllocationInfo stage_allocation_info;
  vmaCreateBuffer(allocator, &buffer_info, &allocation_info, &stage_buffer_, &stage_allocation_,
                  &stage_allocation_info);
  stage_buffer_map_ = stage_allocation_info.pMappedData;
}

Buffer::~Buffer() {
  if (semaphore_) {
    semaphore_->Wait();
  }

  auto allocator = module_->allocator();
  vmaDestroyBuffer(allocator, stage_buffer_, stage_allocation_);
  vmaDestroyBuffer(allocator, buffer_, allocation_);
}

void Buffer::WaitOn(std::shared_ptr<Semaphore> semaphore) {
  if (semaphore_) {
    semaphore_->Wait();
  }

  semaphore_ = semaphore;
}

}  // namespace core
}  // namespace vkgs
