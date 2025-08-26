#include "semaphore_pool.h"

#include "vkgs/core/module.h"

#include "semaphore.h"

namespace vkgs {
namespace core {

SemaphorePool::SemaphorePool(std::shared_ptr<Module> module) : module_(module) {}

SemaphorePool::~SemaphorePool() = default;

std::shared_ptr<Semaphore> SemaphorePool::Allocate() {
  VkSemaphore semaphore;

  if (semaphores_.empty()) {
    VkSemaphoreTypeCreateInfo timeline_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    timeline_semaphore_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timeline_semaphore_info.initialValue = 0;

    VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphore_info.pNext = &timeline_semaphore_info;
    vkCreateSemaphore(module_->device(), &semaphore_info, NULL, &semaphore);
  } else {
    semaphore = semaphores_.back();
    semaphores_.pop_back();
  }

  return std::make_shared<Semaphore>(shared_from_this(), semaphore);
}

void SemaphorePool::Free(VkSemaphore semaphore) { semaphores_.push_back(semaphore); }

}  // namespace core
}  // namespace vkgs
