#ifndef VKGS_IMPL_MODULE_IMPL_H
#define VKGS_IMPL_MODULE_IMPL_H

#include "volk/volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/module.h"
#include "vkgs/buffer.h"

namespace vkgs {

class Module::Impl {
 public:
  Impl();
  ~Impl();

  const std::string& device_name() const noexcept { return device_name_; }
  uint32_t graphics_queue_index() const noexcept { return graphics_queue_index_; }
  uint32_t compute_queue_index() const noexcept { return compute_queue_index_; }
  uint32_t transfer_queue_index() const noexcept { return transfer_queue_index_; }

  auto allocator() const noexcept { return allocator_; }

  void write_buffer(Buffer& buffer, intptr_t ptr);

 private:
  std::string device_name_;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  uint32_t graphics_queue_index_ = VK_QUEUE_FAMILY_IGNORED;
  uint32_t compute_queue_index_ = VK_QUEUE_FAMILY_IGNORED;
  uint32_t transfer_queue_index_ = VK_QUEUE_FAMILY_IGNORED;

  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue compute_queue_ = VK_NULL_HANDLE;
  VkQueue transfer_queue_ = VK_NULL_HANDLE;

  VmaAllocator allocator_ = VK_NULL_HANDLE;

  VkCommandPool transfer_command_pool_ = VK_NULL_HANDLE;
  VkCommandBuffer transfer_command_buffer_ = VK_NULL_HANDLE;
};

}  // namespace vkgs

#endif  // VKGS_IMPL_MODULE_IMPL_H
