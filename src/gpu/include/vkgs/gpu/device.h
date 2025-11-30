#ifndef VKGS_GPU_DEVICE_H
#define VKGS_GPU_DEVICE_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"

#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/queue.h"
#include "vkgs/gpu/semaphore_pool.h"
#include "vkgs/gpu/fence_pool.h"
#include "vkgs/gpu/graphics_pipeline_pool.h"
#include "vkgs/gpu/task_monitor.h"

namespace vkgs {
namespace gpu {

class Object;
class Semaphore;
class Fence;
struct GraphicsPipelineCreateInfo;
class GraphicsPipeline;
class Command;
class QueueTask;
class Task;

struct DeviceCreateInfo {
  bool enable_viewer;
  std::vector<const char*> instance_extensions;
};

class VKGS_GPU_API DeviceImpl : public std::enable_shared_from_this<DeviceImpl> {
 public:
  DeviceImpl(const DeviceCreateInfo& create_info);
  ~DeviceImpl();

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

  QueueTask AddQueueTask(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                         std::function<void()> callback);

 private:
  std::string device_name_;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  void* allocator_ = VK_NULL_HANDLE;

  Queue graphics_queue_;
  Queue compute_queue_;
  Queue transfer_queue_;
  SemaphorePool semaphore_pool_;
  FencePool fence_pool_;
  GraphicsPipelinePool graphics_pipeline_pool_;
  TaskMonitor task_monitor_;

  Task* current_task_ = nullptr;
};

class VKGS_GPU_API Device : public SharedAccessor<Device, DeviceImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DEVICE_H
