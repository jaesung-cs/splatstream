#ifndef VKGS_CORE_MODULE_H
#define VKGS_CORE_MODULE_H

#include <string>
#include <cstdint>
#include <memory>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class Buffer;
class CommandPool;

class VKGS_CORE_API Module : public std::enable_shared_from_this<Module> {
 public:
  Module();
  ~Module();

  void Init();

  const std::string& device_name() const noexcept { return device_name_; }
  uint32_t graphics_queue_index() const noexcept { return graphics_queue_index_; }
  uint32_t compute_queue_index() const noexcept { return compute_queue_index_; }
  uint32_t transfer_queue_index() const noexcept { return transfer_queue_index_; }

  auto allocator() const noexcept { return allocator_; }
  auto device() const noexcept { return device_; }

  void WaitIdle();
  void WriteBuffer(std::shared_ptr<Buffer> buffer, void* ptr);

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

  std::shared_ptr<CommandPool> graphics_command_pool_;
  std::shared_ptr<CommandPool> compute_command_pool_;
  std::shared_ptr<CommandPool> transfer_command_pool_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_MODULE_H
