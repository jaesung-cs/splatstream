#include "details/semaphore_pool.h"

#include <vector>

#include "volk.h"

#include "vkgs/gpu/semaphore.h"

namespace vkgs {
namespace gpu {

class SemaphorePoolImpl : public EnableHandleFromThis<SemaphorePool, SemaphorePoolImpl> {
 public:
  void __init__(VkDevice device) { device_ = device; }

  void __del__() {
    for (auto semaphore : semaphores_) {
      vkDestroySemaphore(device_, semaphore.first, NULL);
    }
  }

  Semaphore Allocate() {
    std::pair<VkSemaphore, uint64_t> semaphore;

    if (semaphores_.empty()) {
      VkSemaphoreTypeCreateInfo timeline_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
      timeline_semaphore_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
      timeline_semaphore_info.initialValue = 0;

      VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      semaphore_info.pNext = &timeline_semaphore_info;
      vkCreateSemaphore(device_, &semaphore_info, NULL, &semaphore.first);
      semaphore.second = 0;
    } else {
      semaphore = semaphores_.back();
      semaphores_.pop_back();
    }

    return Semaphore::Create(HandleFromThis(), semaphore.first, semaphore.second);
  }

  void Free(VkSemaphore semaphore, uint64_t value) { semaphores_.emplace_back(semaphore, value); }

 private:
  VkDevice device_;

  std::vector<std::pair<VkSemaphore, uint64_t>> semaphores_;
};

SemaphorePool SemaphorePool::Create(VkDevice device) { return Make<SemaphorePoolImpl>(device); }

Semaphore SemaphorePool::Allocate() { return impl_->Allocate(); }
void SemaphorePool::Free(VkSemaphore semaphore, uint64_t value) { impl_->Free(semaphore, value); }

}  // namespace gpu
}  // namespace vkgs
