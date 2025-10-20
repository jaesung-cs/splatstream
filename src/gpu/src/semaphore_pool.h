#ifndef VKGS_GPU_SEMAPHORE_POOL_H
#define VKGS_GPU_SEMAPHORE_POOL_H

#include <memory>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace gpu {

class Semaphore;

class SemaphorePool : public std::enable_shared_from_this<SemaphorePool> {
 public:
  explicit SemaphorePool(VkDevice device);
  ~SemaphorePool();

  std::shared_ptr<Semaphore> Allocate();
  void Free(VkSemaphore semaphore, uint64_t value);

 private:
  VkDevice device_;

  std::vector<std::pair<VkSemaphore, uint64_t>> semaphores_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SEMAPHORE_POOL_H
