#include "vkgs/gpu/semaphore.h"

#include "volk.h"

#include "vkgs/gpu/object.h"

#include "details/semaphore_pool.h"

namespace vkgs {
namespace gpu {

class SemaphoreImpl : public Object {
 public:
  void __init__() {
    semaphore_pool_ = device_.semaphore_pool();
    allocation_ = semaphore_pool_.Allocate();
  }

  void __del__() {
    Wait();
    semaphore_pool_.Free(allocation_);
  }

  operator VkSemaphore() const noexcept { return allocation_.semaphore; }
  auto value() const noexcept { return allocation_.value; }

  uint64_t operator+(int value) { return allocation_.value + value; }
  uint64_t operator-(int value) { return allocation_.value - value; }
  bool operator>=(uint64_t value) { return allocation_.value >= value; }

  void Wait() {
    VkSemaphoreWaitInfo wait_info = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &allocation_.semaphore;
    wait_info.pValues = &allocation_.value;
    vkWaitSemaphores(device_, &wait_info, UINT64_MAX);
  }

  void SetValue(uint64_t value) { allocation_.value = value; }

  SemaphoreImpl& operator++() {
    ++allocation_.value;
    return *this;
  }

  void operator++(int) { allocation_.value++; }

  SemaphoreImpl& operator+=(int value) {
    allocation_.value += value;
    return *this;
  }

 private:
  SemaphorePool semaphore_pool_;
  SemaphoreAllocation allocation_;
};

Semaphore Semaphore::Create() { return Make<SemaphoreImpl>(); }

void Semaphore::Keep() { impl_->Keep(); }

Semaphore::operator VkSemaphore() const { return *impl_; }
uint64_t Semaphore::value() const { return impl_->value(); }

uint64_t Semaphore::operator+(int value) { return (*impl_) + value; }
uint64_t Semaphore::operator-(int value) { return (*impl_) - value; }
bool Semaphore::operator>=(uint64_t value) { return (*impl_) >= value; }

void Semaphore::Wait() { impl_->Wait(); }
void Semaphore::SetValue(uint64_t value) { impl_->SetValue(value); }

Semaphore& Semaphore::operator++() {
  ++*impl_;
  return *this;
}

void Semaphore::operator++(int) { (*impl_)++; }

Semaphore& Semaphore::operator+=(int value) {
  *impl_ += value;
  return *this;
}

}  // namespace gpu
}  // namespace vkgs
