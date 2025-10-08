#ifndef VKGS_CORE_SEMAPHORE_H
#define VKGS_CORE_SEMAPHORE_H

#include <memory>

#include "volk.h"

namespace vkgs {
namespace core {

class SemaphorePool;

class Semaphore {
 public:
  Semaphore(VkDevice device, std::shared_ptr<SemaphorePool> semaphore_pool, VkSemaphore semaphore, uint64_t value);
  ~Semaphore();

  VkSemaphore semaphore() const noexcept { return semaphore_; }
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

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_SEMAPHORE_H
