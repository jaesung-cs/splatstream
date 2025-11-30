#ifndef VKGS_GPU_DEVICE_H
#define VKGS_GPU_DEVICE_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Object;
class Queue;
class Semaphore;
class Fence;
struct GraphicsPipelineCreateInfo;
class GraphicsPipeline;
class SemaphorePool;
class FencePool;
class TaskMonitor;
class Command;
class QueueTask;
class Task;
class GraphicsPipelinePool;

struct DeviceCreateInfo {
  bool enable_viewer;
  std::vector<const char*> instance_extensions;
};

class VKGS_GPU_API Device : public std::enable_shared_from_this<Device> {
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

  Semaphore AllocateSemaphore();
  Fence AllocateFence();
  GraphicsPipeline AllocateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);

  void WaitIdle();

  // Internal
  void SetCurrentTask(Task* task) { current_task_ = task; }
  void ClearCurrentTask() { current_task_ = nullptr; }
  Task* CurrentTask() const { return current_task_; }

  std::shared_ptr<QueueTask> AddQueueTask(Fence fence, std::shared_ptr<Command> command,
                                          std::vector<std::shared_ptr<Object>> objects, std::function<void()> callback);

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
  std::shared_ptr<GraphicsPipelinePool> graphics_pipeline_pool_;
  std::shared_ptr<TaskMonitor> task_monitor_;

  Task* current_task_ = nullptr;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DEVICE_H
