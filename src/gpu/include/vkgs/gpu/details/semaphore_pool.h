#ifndef VKGS_GPU_DETAILS_SEMAPHORE_POOL_H
#define VKGS_GPU_DETAILS_SEMAPHORE_POOL_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace gpu {

class Semaphore;

class SemaphorePoolImpl;
class SemaphorePool : public Handle<SemaphorePool, SemaphorePoolImpl> {
 public:
  static SemaphorePool Create(VkDevice device);

  Semaphore Allocate();
  void Free(VkSemaphore semaphore, uint64_t value);
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_SEMAPHORE_POOL_H
