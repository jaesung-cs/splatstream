#ifndef VKGS_GPU_SEMAPHORE_POOL_H
#define VKGS_GPU_SEMAPHORE_POOL_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"

namespace vkgs {
namespace gpu {

class Semaphore;

class SemaphorePoolImpl : public std::enable_shared_from_this<SemaphorePoolImpl> {
 public:
  explicit SemaphorePoolImpl(VkDevice device);
  ~SemaphorePoolImpl();

  Semaphore Allocate();
  void Free(VkSemaphore semaphore, uint64_t value);

 private:
  VkDevice device_;

  std::vector<std::pair<VkSemaphore, uint64_t>> semaphores_;
};

class SemaphorePool : public SharedAccessor<SemaphorePool, SemaphorePoolImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SEMAPHORE_POOL_H
