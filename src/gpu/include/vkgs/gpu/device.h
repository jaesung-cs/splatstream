#ifndef VKGS_GPU_DEVICE_H
#define VKGS_GPU_DEVICE_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

struct VmaAllocator_T;

namespace vkgs {
namespace gpu {

class Object;
class Semaphore;
class Fence;
struct GraphicsPipelineCreateInfo;
class GraphicsPipeline;
class Command;
class QueueTask;
class Queue;
class Task;

struct DeviceCreateInfo {
  bool enable_viewer;
  std::vector<const char*> instance_extensions;
};

class DeviceImpl;
class VKGS_GPU_API Device : public Handle<Device, DeviceImpl> {
 public:
  static Device Create(const DeviceCreateInfo& create_info);

  operator VkDevice() const noexcept;

  const std::string& device_name() const noexcept;
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  VkInstance instance() const noexcept;
  VmaAllocator_T* allocator() const noexcept;
  VkPhysicalDevice physical_device() const noexcept;
  VkDevice device() const noexcept;

  Queue graphics_queue() const noexcept;
  Queue compute_queue() const noexcept;
  Queue transfer_queue() const noexcept;

  Semaphore AllocateSemaphore();
  Fence AllocateFence();
  GraphicsPipeline AllocateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);

  void WaitIdle();

  // Internal
  void SetCurrentTask(Task* task);
  void ClearCurrentTask();
  Task* CurrentTask() const;

  QueueTask AddQueueTask(Fence fence, Command command, std::vector<std::shared_ptr<Object>> objects,
                         std::function<void()> callback);
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DEVICE_H
