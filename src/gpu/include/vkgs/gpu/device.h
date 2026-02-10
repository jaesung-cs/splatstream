#ifndef VKGS_GPU_DEVICE_H
#define VKGS_GPU_DEVICE_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/queue_type.h"

struct VmaAllocator_T;

namespace vkgs {
namespace gpu {

class Fence;
struct GraphicsPipelineCreateInfo;
class GraphicsPipeline;
class Command;
class CommandBuffer;
class QueueTask;
class Queue;
class SemaphorePool;
class FencePool;
class TimerPool;

class DeviceImpl;
class VKGS_GPU_API Device : public Handle<Device, DeviceImpl> {
 public:
  static Device Create();

  operator VkPhysicalDevice() const noexcept;
  operator VkDevice() const noexcept;

  const std::string& device_name() const noexcept;

  VkInstance instance() const noexcept;
  VmaAllocator_T* allocator() const noexcept;

  Queue queue(QueueType queue_type) const;
  Queue graphics_queue() const;
  Queue compute_queue() const;
  Queue transfer_queue() const;

  GraphicsPipeline AllocateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);

  void WaitIdle();

  // Internal
  SemaphorePool semaphore_pool() const;
  FencePool fence_pool() const;
  TimerPool timer_pool() const;

  void SetCurrentCommand(Command* command);
  void ClearCurrentCommand();
  Command* CurrentCommand() const;

  QueueTask AddQueueTask(Fence fence, CommandBuffer cb, std::vector<AnyHandle> objects, std::function<void()> callback);
};

Device VKGS_GPU_API GetDevice();

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DEVICE_H
