#include "vkgs/gpu/semaphore.h"

#include <volk.h>

#include "semaphore_pool.h"

namespace vkgs {
namespace gpu {

SemaphoreImpl::SemaphoreImpl(std::shared_ptr<SemaphorePool> semaphore_pool, VkSemaphore semaphore, uint64_t value)
    : semaphore_pool_(semaphore_pool), semaphore_(semaphore), value_(value) {}

SemaphoreImpl::~SemaphoreImpl() {
  Wait();
  semaphore_pool_->Free(semaphore_, value_);
}

void SemaphoreImpl::Wait() {
  VkSemaphoreWaitInfo wait_info = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
  wait_info.semaphoreCount = 1;
  wait_info.pSemaphores = &semaphore_;
  wait_info.pValues = &value_;
  vkWaitSemaphores(device_, &wait_info, UINT64_MAX);
}

void SemaphoreImpl::SetValue(uint64_t value) { value_ = value; }

}  // namespace gpu
}  // namespace vkgs
