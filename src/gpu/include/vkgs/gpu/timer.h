#ifndef VKGS_GPU_TIMER_H
#define VKGS_GPU_TIMER_H

#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class TimerImpl;
class VKGS_GPU_API Timer : public Handle<Timer, TimerImpl> {
 public:
  static Timer Create(uint32_t size);

  void Record(VkCommandBuffer cb, VkPipelineStageFlags2 stage);
  std::vector<uint64_t> GetTimestamps() const;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TIMER_H
