#ifndef VKGS_GPU_SEMAPHORE_H
#define VKGS_GPU_SEMAPHORE_H

#include <memory>

#include <vulkan/vulkan.h>

#include "export_api.h"
#include "object.h"

namespace vkgs {
namespace gpu {

class SemaphorePool;

class VKGS_GPU_API SemaphoreImpl : public Object {
 public:
  SemaphoreImpl(std::shared_ptr<SemaphorePool> semaphore_pool, VkSemaphore semaphore, uint64_t value);
  ~SemaphoreImpl();

  operator VkSemaphore() const noexcept { return semaphore_; }
  auto value() const noexcept { return value_; }

  void Wait();

  void SetValue(uint64_t value);
  void Increment() { value_++; }

 private:
  std::shared_ptr<SemaphorePool> semaphore_pool_;

  VkSemaphore semaphore_;
  uint64_t value_;
};

class VKGS_GPU_API Semaphore : public SharedAccessor<Semaphore, SemaphoreImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SEMAPHORE_H
