#include "semaphore.h"

#include "vkgs/core/module.h"

#include "semaphore_pool.h"

namespace vkgs {
namespace core {

Semaphore::Semaphore(std::shared_ptr<SemaphorePool> semaphore_pool, VkSemaphore semaphore)
    : semaphore_pool_(semaphore_pool), semaphore_(semaphore) {}

Semaphore::~Semaphore() { semaphore_pool_->Free(semaphore_); }

void Semaphore::Wait() {
  VkSemaphoreWaitInfo wait_info = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
  wait_info.semaphoreCount = 1;
  wait_info.pSemaphores = &semaphore_;
  wait_info.pValues = &value_;
  vkWaitSemaphores(semaphore_pool_->module()->device(), &wait_info, UINT64_MAX);

  // Successful wait of this semaphore guarantees the dependent wait semaphores, so release ownership.
  wait_semaphores_.clear();
}

void Semaphore::SignalBy(std::vector<std::shared_ptr<Semaphore>> wait_semaphores, std::shared_ptr<Command> command,
                         uint64_t value) {
  wait_semaphores_ = std::move(wait_semaphores);
  command_ = command;
  value_ = value;
}

}  // namespace core
}  // namespace vkgs
