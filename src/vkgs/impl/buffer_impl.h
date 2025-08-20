#include "vkgs/buffer.h"

#include "volk/volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/impl/module_impl.h"

namespace vkgs {

class Buffer::Impl {
 public:
  Impl(Module* module, size_t size) : module_(module), size_(size) {
    auto allocator = module_->impl()->allocator();
    VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer(allocator, &buffer_info, &allocation_info, &buffer_, &allocation_, nullptr);

    // Stage buffer
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    allocation_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocation_info.usage = VMA_MEMORY_USAGE_AUTO;
    vmaCreateBuffer(allocator, &buffer_info, &allocation_info, &stage_buffer_, &stage_allocation_, nullptr);
  }

  ~Impl() {
    auto allocator = module_->impl()->allocator();
    vmaDestroyBuffer(allocator, stage_buffer_, stage_allocation_);
    vmaDestroyBuffer(allocator, buffer_, allocation_);
  }

  VkBuffer handle() const noexcept { return buffer_; }

  size_t size() const noexcept { return size_; }

 private:
  Module* module_ = nullptr;
  size_t size_ = 0;

  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;

  VkBuffer stage_buffer_ = VK_NULL_HANDLE;
  VmaAllocation stage_allocation_ = VK_NULL_HANDLE;
};

}  // namespace vkgs
