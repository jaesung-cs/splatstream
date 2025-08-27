#ifndef VKGS_CORE_SEMAPHORE_H
#define VKGS_CORE_SEMAPHORE_H

#include <memory>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace core {

class SemaphorePool;
class Command;

class Semaphore {
 public:
  Semaphore(std::shared_ptr<SemaphorePool> semaphore_pool, VkSemaphore semaphore);
  ~Semaphore();

  VkSemaphore semaphore() const noexcept { return semaphore_; }
  auto value() const noexcept { return value_; }

  void Wait();
  void SignalBy(std::vector<std::shared_ptr<Semaphore>> wait_semaphores, std::shared_ptr<Command> command,
                uint64_t value);

 private:
  std::shared_ptr<SemaphorePool> semaphore_pool_;

  VkSemaphore semaphore_ = VK_NULL_HANDLE;
  uint64_t value_ = 0;

  // Command buffer that waits on this semaphore.
  std::shared_ptr<Command> command_;

  // Semaphore chain to hold the ownership.
  std::vector<std::shared_ptr<Semaphore>> wait_semaphores_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_SEMAPHORE_H
