#ifndef VKGS_IMPL_BUFFER_IMPL_H
#define VKGS_IMPL_BUFFER_IMPL_H

#include "vkgs/buffer.h"

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/module.h"

namespace vkgs {

class Buffer::Impl {
 public:
  Impl(Module module, size_t size);
  ~Impl();

  VkBuffer buffer() const noexcept { return buffer_; }
  VkBuffer stage_buffer() const noexcept { return stage_buffer_; }
  void* stage_buffer_map() const noexcept { return stage_buffer_map_; }

  size_t size() const noexcept { return size_; }

 private:
  Module module_;
  size_t size_ = 0;

  VkBuffer buffer_ = VK_NULL_HANDLE;
  VmaAllocation allocation_ = VK_NULL_HANDLE;

  VkBuffer stage_buffer_ = VK_NULL_HANDLE;
  VmaAllocation stage_allocation_ = VK_NULL_HANDLE;
  void* stage_buffer_map_ = nullptr;
};

}  // namespace vkgs

#endif  // VKGS_IMPL_BUFFER_IMPL_H
