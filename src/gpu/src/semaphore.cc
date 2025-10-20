#include "vkgs/gpu/semaphore.h"

#include "semaphore_pool.h"

namespace vkgs {
namespace gpu {

Semaphore::Semaphore(VkDevice device, std::shared_ptr<SemaphorePool> semaphore_pool, VkSemaphore semaphore,
                     uint64_t value)
    : device_(device), semaphore_pool_(semaphore_pool), semaphore_(semaphore), value_(value) {}

Semaphore::~Semaphore() {
  Wait();
  semaphore_pool_->Free(semaphore_, value_);
}

void Semaphore::Wait() {
  VkSemaphoreWaitInfo wait_info = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
  wait_info.semaphoreCount = 1;
  wait_info.pSemaphores = &semaphore_;
  wait_info.pValues = &value_;
  vkWaitSemaphores(device_, &wait_info, UINT64_MAX);
}

void Semaphore::SetValue(uint64_t value) { value_ = value; }

}  // namespace gpu
}  // namespace vkgs
