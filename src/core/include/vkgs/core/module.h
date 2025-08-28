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
class Queue;
class CommandPool;
class SemaphorePool;
class FencePool;
class TaskMonitor;
class Sorter;

class VKGS_CORE_API Module : public std::enable_shared_from_this<Module> {
 public:
  Module();
  ~Module();

  void Init();

  const std::string& device_name() const noexcept { return device_name_; }
  uint32_t graphics_queue_index() const;
  uint32_t compute_queue_index() const;
  uint32_t transfer_queue_index() const;

  auto allocator() const noexcept { return allocator_; }
  auto physical_device() const noexcept { return physical_device_; }
  auto device() const noexcept { return device_; }

  void WaitIdle();
  void CpuToBuffer(std::shared_ptr<Buffer> buffer, const void* ptr, size_t size);
  void BufferToCpu(std::shared_ptr<Buffer> buffer, void* ptr, size_t size);
  void FillBuffer(std::shared_ptr<Buffer> buffer, uint32_t value);
  void SortBuffer(std::shared_ptr<Buffer> buffer);

 private:
  void TransferOwnership(std::shared_ptr<Buffer> buffer, std::shared_ptr<Queue> dst_queue,
                         VkPipelineStageFlags2 dst_stage_mask, VkAccessFlags2 dst_access_mask);

  std::string device_name_;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VmaAllocator allocator_ = VK_NULL_HANDLE;

  std::shared_ptr<Queue> graphics_queue_;
  std::shared_ptr<Queue> compute_queue_;
  std::shared_ptr<Queue> transfer_queue_;
  std::shared_ptr<CommandPool> graphics_command_pool_;
  std::shared_ptr<CommandPool> compute_command_pool_;
  std::shared_ptr<CommandPool> transfer_command_pool_;
  std::shared_ptr<SemaphorePool> semaphore_pool_;
  std::shared_ptr<FencePool> fence_pool_;
  std::shared_ptr<TaskMonitor> task_monitor_;

  std::shared_ptr<Sorter> sorter_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_MODULE_H
