#ifndef VKGS_CORE_MODULE_H
#define VKGS_CORE_MODULE_H

#include <string>
#include <cstdint>
#include <memory>
#include <vector>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class SemaphorePool;
class FencePool;
class TaskMonitor;
class Sorter;
class Queue;

class VKGS_CORE_API Module {
 public:
  Module();
  ~Module();

  const std::string& device_name() const noexcept { return device_name_; }
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  auto allocator() const noexcept { return allocator_; }
  auto physical_device() const noexcept { return physical_device_; }
  auto device() const noexcept { return device_; }
  auto semaphore_pool() const noexcept { return semaphore_pool_; }

  void WaitIdle();

 private:
  std::string device_name_;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  VmaAllocator allocator_ = VK_NULL_HANDLE;

  std::shared_ptr<Queue> graphics_queue_;
  std::shared_ptr<Queue> compute_queue_;
  std::shared_ptr<Queue> transfer_queue_;
  std::shared_ptr<SemaphorePool> semaphore_pool_;
  std::shared_ptr<FencePool> fence_pool_;
  std::shared_ptr<TaskMonitor> task_monitor_;

  std::shared_ptr<Sorter> sorter_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_MODULE_H
