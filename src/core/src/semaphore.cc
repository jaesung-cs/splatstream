#include "semaphore.h"

#include "vkgs/core/module.h"

namespace vkgs {
namespace core {

Semaphore::Semaphore(std::shared_ptr<Module> module) : module_(module) {
  VkSemaphoreTypeCreateInfo timeline_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
  timeline_semaphore_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  timeline_semaphore_info.initialValue = 0;

  VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  semaphore_info.pNext = &timeline_semaphore_info;
  vkCreateSemaphore(module_->device(), &semaphore_info, NULL, &semaphore_);
}

Semaphore::~Semaphore() {
  Wait();
  vkDestroySemaphore(module_->device(), semaphore_, NULL);
}

void Semaphore::Wait() {
  VkSemaphoreWaitInfo wait_info = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
  wait_info.semaphoreCount = 1;
  wait_info.pSemaphores = &semaphore_;
  wait_info.pValues = &value_;
  vkWaitSemaphores(module_->device(), &wait_info, UINT64_MAX);
}

void Semaphore::SignalBy(std::shared_ptr<Command> command, uint64_t value) {
  command_ = command;
  value_ = value;
}

}  // namespace core
}  // namespace vkgs
