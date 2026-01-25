#ifndef VKGS_GPU_SEMAPHORE_H
#define VKGS_GPU_SEMAPHORE_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class SemaphorePool;

class SemaphoreImpl;
class VKGS_GPU_API Semaphore : public Handle<Semaphore, SemaphoreImpl> {
 public:
  static Semaphore Create(SemaphorePool semaphore_pool, VkSemaphore semaphore, uint64_t value);

  operator VkSemaphore() const;
  uint64_t value() const;

  void Wait();

  void SetValue(uint64_t value);
  void Increment();
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SEMAPHORE_H
