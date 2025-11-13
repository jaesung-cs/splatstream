#ifndef VKGS_CORE_DEVICE_H
#define VKGS_CORE_DEVICE_H

#include <memory>

#include <volk.h>

namespace vkgs {
namespace core {

class Device {
 public:
  Device();
  ~Device();

  VkInstance instance() const noexcept { return instance_; }
  VkPhysicalDevice physical_device() const noexcept { return physical_device_; }
  VkDevice device() const noexcept { return device_; }
  uint32_t graphics_queue_index() const noexcept { return graphics_queue_index_; }
  uint32_t compute_queue_index() const noexcept { return compute_queue_index_; }
  uint32_t transfer_queue_index() const noexcept { return transfer_queue_index_; }
  VkQueue graphics_queue() const noexcept { return graphics_queue_; }
  VkQueue compute_queue() const noexcept { return compute_queue_; }
  VkQueue transfer_queue() const noexcept { return transfer_queue_; }

 private:
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
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_DEVICE_H
