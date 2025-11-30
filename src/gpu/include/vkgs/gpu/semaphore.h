#ifndef VKGS_GPU_SEMAPHORE_H
#define VKGS_GPU_SEMAPHORE_H

#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class SemaphorePool;

class VKGS_GPU_API SemaphoreImpl : public Object {
 public:
  SemaphoreImpl(SemaphorePool semaphore_pool, VkSemaphore semaphore, uint64_t value);
  ~SemaphoreImpl() override;

  operator VkSemaphore() const noexcept { return semaphore_; }
  auto value() const noexcept { return value_; }

  void Wait();

  void SetValue(uint64_t value);
  void Increment() { value_++; }

 private:
  SemaphorePool semaphore_pool_;

  VkSemaphore semaphore_;
  uint64_t value_;
};

class VKGS_GPU_API Semaphore : public SharedAccessor<Semaphore, SemaphoreImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SEMAPHORE_H
