#ifndef VKGS_GPU_DEVICE_H
#define VKGS_GPU_DEVICE_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

#include "export_api.h"

#include "queue_submission.h"

namespace vkgs {
namespace gpu {

class Queue;
class Semaphore;
class Fence;
class SemaphorePool;
class FencePool;
class TaskMonitor;
class Command;

struct DeviceCreateInfo {
  std::vector<const char*> instance_extensions;
};

class VKGS_GPU_API Device : public std::enable_shared_from_this<Device> {
 private:
  using TaskCallback = std::function<void(VkCommandBuffer)>;

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

  QueueSubmission ComputeTask(TaskCallback task_callback, std::function<void()> host_callback = {});
  QueueSubmission GraphicsTask(TaskCallback task_callback, std::function<void()> host_callback = {});
  QueueSubmission TransferTask(TaskCallback task_callback, std::function<void()> host_callback = {});

  void WaitIdle();

  // Internal
  std::shared_ptr<Task> AddTask(std::shared_ptr<Fence> fence, std::shared_ptr<Command> command,
                                std::function<void(VkCommandBuffer)> task_callback,
                                std::function<void()> host_callback);

 private:
  QueueSubmission PrepareTask(std::shared_ptr<Queue> queue, TaskCallback task_callback,
                              std::function<void()> host_callback);

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
  std::shared_ptr<TaskMonitor> task_monitor_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DEVICE_H
