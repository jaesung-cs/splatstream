#ifndef VKGS_GPU_DEVICE_H
#define VKGS_GPU_DEVICE_H

#include <string>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Queue;
class Semaphore;
class Fence;
class SemaphorePool;
class FencePool;

struct DeviceCreateInfo {
  std::vector<const char*> instance_extensions;
};

class VKGS_GPU_API Device {
 public:
  Device(const DeviceCreateInfo& create_info);
  ~Device();

  operator VkDevice() const noexcept { return device_; }

  const std::string& device_name() const noexcept { return device_name_; }
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  auto instance() const noexcept { return instance_; }
  auto allocator() const noexcept { return allocator_; }
  auto physical_device() const noexcept { return physical_device_; }
  auto device() const noexcept { return device_; }

  auto graphics_queue() const noexcept { return graphics_queue_; }
  auto compute_queue() const noexcept { return compute_queue_; }
  auto transfer_queue() const noexcept { return transfer_queue_; }

  std::shared_ptr<Semaphore> AllocateSemaphore();
  std::shared_ptr<Fence> AllocateFence();

  void WaitIdle();

 private:
  std::string device_name_;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  void* allocator_ = VK_NULL_HANDLE;

  std::shared_ptr<Queue> graphics_queue_;
  std::shared_ptr<Queue> compute_queue_;
  std::shared_ptr<Queue> transfer_queue_;
  std::shared_ptr<SemaphorePool> semaphore_pool_;
  std::shared_ptr<FencePool> fence_pool_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DEVICE_H
