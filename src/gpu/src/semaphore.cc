#include "vkgs/gpu/semaphore.h"

#include "volk.h"

#include "vkgs/gpu/object.h"

#include "details/semaphore_pool.h"

namespace vkgs {
namespace gpu {

class SemaphoreImpl : public Object {
 public:
  void __init__(SemaphorePool semaphore_pool, VkSemaphore semaphore, uint64_t value) {
    semaphore_pool_ = semaphore_pool;
    semaphore_ = semaphore;
    value_ = value;
  }

  void __del__() {
    Wait();
    semaphore_pool_.Free(semaphore_, value_);
  }

  operator VkSemaphore() const noexcept { return semaphore_; }
  auto value() const noexcept { return value_; }

  void Wait() {
    VkSemaphoreWaitInfo wait_info = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &semaphore_;
    wait_info.pValues = &value_;
    vkWaitSemaphores(device_, &wait_info, UINT64_MAX);
  }

  void SetValue(uint64_t value) { value_ = value; }
  void Increment() { value_++; }

 private:
  SemaphorePool semaphore_pool_;

  VkSemaphore semaphore_;
  uint64_t value_;
};

Semaphore Semaphore::Create(SemaphorePool semaphore_pool, VkSemaphore semaphore, uint64_t value) {
  return Make<SemaphoreImpl>(semaphore_pool, semaphore, value);
}

Semaphore::operator VkSemaphore() const { return *impl_; }
uint64_t Semaphore::value() const { return impl_->value(); }
void Semaphore::Wait() { impl_->Wait(); }
void Semaphore::SetValue(uint64_t value) { impl_->SetValue(value); }
void Semaphore::Increment() { impl_->Increment(); }

}  // namespace gpu
}  // namespace vkgs
