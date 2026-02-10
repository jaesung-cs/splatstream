#include "details/semaphore_pool.h"

#include <vector>

#include "volk.h"

namespace vkgs {
namespace gpu {

class SemaphorePoolImpl {
 public:
  void __init__(VkDevice device) { device_ = device; }

  void __del__() {
    for (const auto& allocation : allocations_) {
      vkDestroySemaphore(device_, allocation.semaphore, NULL);
    }
  }

  SemaphoreAllocation Allocate() {
    SemaphoreAllocation allocation;

    if (allocations_.empty()) {
      VkSemaphoreTypeCreateInfo timeline_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
      timeline_semaphore_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
      timeline_semaphore_info.initialValue = 0;

      VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      semaphore_info.pNext = &timeline_semaphore_info;
      vkCreateSemaphore(device_, &semaphore_info, NULL, &allocation.semaphore);
      allocation.value = 0;
    } else {
      allocation = allocations_.back();
      allocations_.pop_back();
    }

    return allocation;
  }

  void Free(const SemaphoreAllocation& allocation) { allocations_.emplace_back(allocation); }

 private:
  VkDevice device_;

  std::vector<SemaphoreAllocation> allocations_;
};

SemaphorePool SemaphorePool::Create(VkDevice device) { return Make<SemaphorePoolImpl>(device); }

SemaphoreAllocation SemaphorePool::Allocate() { return impl_->Allocate(); }
void SemaphorePool::Free(const SemaphoreAllocation& allocation) { impl_->Free(allocation); }

}  // namespace gpu
}  // namespace vkgs
