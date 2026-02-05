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

class Semaphore;
class Fence;
struct GraphicsPipelineCreateInfo;
class GraphicsPipeline;
class Command;
class CommandBuffer;
class QueueTask;
class Queue;

class DeviceImpl;
class VKGS_GPU_API Device : public Handle<Device, DeviceImpl> {
 public:
  static Device Create();

  operator VkPhysicalDevice() const noexcept;
  operator VkDevice() const noexcept;

  const std::string& device_name() const noexcept;

  VkInstance instance() const noexcept;
  VmaAllocator_T* allocator() const noexcept;

  Queue graphics_queue() const noexcept;
  Queue compute_queue() const noexcept;
  Queue transfer_queue() const noexcept;

  Semaphore AllocateSemaphore();
  Fence AllocateFence();
  GraphicsPipeline AllocateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);

  void WaitIdle();

  // Internal
  void SetCurrentCommand(Command* command);
  void ClearCurrentCommand();
  Command* CurrentCommand() const;

  QueueTask AddQueueTask(Fence fence, CommandBuffer cb, std::vector<AnyHandle> objects, std::function<void()> callback);
};

Device VKGS_GPU_API GetDevice();

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DEVICE_H
