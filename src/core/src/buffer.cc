#include "vkgs/core/buffer.h"

#include "vkgs/core/module.h"

#include "semaphore.h"

namespace vkgs {
namespace core {

Buffer::Buffer(std::shared_ptr<Module> module, size_t size) : module_(module), size_(size) {
  auto allocator = module->allocator();
  VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  buffer_info.size = size;
  buffer_info.usage =
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
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
  WaitForWrite();

  auto allocator = module_->allocator();
  vmaDestroyBuffer(allocator, stage_buffer_, stage_allocation_);
  vmaDestroyBuffer(allocator, buffer_, allocation_);
}

void Buffer::ToGpu(const void* ptr, size_t size) { module_->CpuToBuffer(shared_from_this(), ptr, size); }

void Buffer::ToCpu(void* ptr, size_t size) { module_->BufferToCpu(shared_from_this(), ptr, size); }

void Buffer::Fill(uint32_t value) { module_->FillBuffer(shared_from_this(), value); }

void Buffer::Sort() { module_->SortBuffer(shared_from_this()); }

void Buffer::WaitForRead() {
  for (auto& write_wait_info : write_wait_infos_) {
    write_wait_info.semaphore->Wait();
  }
  write_wait_infos_.clear();
}

void Buffer::WaitForWrite() {
  for (auto& write_wait_info : write_wait_infos_) {
    write_wait_info.semaphore->Wait();
  }
  write_wait_infos_.clear();

  for (auto& read_wait_info : read_wait_infos_) {
    read_wait_info.semaphore->Wait();
  }
  read_wait_infos_.clear();
}

void Buffer::ClearWaitInfo() {
  write_wait_infos_.clear();
  read_wait_infos_.clear();
}

void Buffer::WaitOnWrite(std::shared_ptr<Semaphore> semaphore, VkPipelineStageFlags2 stage_mask,
                         VkAccessFlags2 access_mask) {
  write_wait_infos_.emplace_back(WriteWaitInfo{semaphore, stage_mask, access_mask});
}

void Buffer::WaitOnRead(std::shared_ptr<Semaphore> semaphore, VkPipelineStageFlags2 stage_mask) {
  read_wait_infos_.emplace_back(ReadWaitInfo{semaphore, stage_mask});
}

}  // namespace core
}  // namespace vkgs
