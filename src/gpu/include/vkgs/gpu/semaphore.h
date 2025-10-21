#ifndef VKGS_GPU_SEMAPHORE_H
#define VKGS_GPU_SEMAPHORE_H

#include "object.h"

#include <memory>

#include "volk.h"

#include "export_api.h"

namespace vkgs {
namespace gpu {

class SemaphorePool;

class Semaphore : public Object {
 public:
  Semaphore(VkDevice device, std::shared_ptr<SemaphorePool> semaphore_pool, VkSemaphore semaphore, uint64_t value);
  ~Semaphore() override;

  operator VkSemaphore() const noexcept { return semaphore_; }
  auto value() const noexcept { return value_; }

  void Wait();

  void SetValue(uint64_t value);
  void Increment() { value_++; }

 private:
  VkDevice device_;
  std::shared_ptr<SemaphorePool> semaphore_pool_;

  VkSemaphore semaphore_;
  uint64_t value_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SEMAPHORE_H
