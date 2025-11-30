#include "semaphore_pool.h"

#include <volk.h>

#include "vkgs/gpu/semaphore.h"

namespace vkgs {
namespace gpu {

SemaphorePool::SemaphorePool(VkDevice device) : device_(device) {}

SemaphorePool::~SemaphorePool() {
  for (auto semaphore : semaphores_) {
    vkDestroySemaphore(device_, semaphore.first, NULL);
  }
}

Semaphore SemaphorePool::Allocate() {
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

  return Semaphore::Create(shared_from_this(), semaphore.first, semaphore.second);
}

void SemaphorePool::Free(VkSemaphore semaphore, uint64_t value) { semaphores_.emplace_back(semaphore, value); }

}  // namespace gpu
}  // namespace vkgs
