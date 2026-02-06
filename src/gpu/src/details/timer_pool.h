#ifndef VKGS_GPU_DETAILS_TIMER_POOL_H
#define VKGS_GPU_DETAILS_TIMER_POOL_H

#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class Device;
class Timer;

class TimerPoolImpl;
class TimerPool : public Handle<TimerPool, TimerPoolImpl> {
 public:
  static TimerPool Create(Device device);

  Timer Allocate(uint32_t size);
  void Free(VkQueryPool query_pool, uint32_t start, uint32_t size);
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_TIMER_POOL_H
