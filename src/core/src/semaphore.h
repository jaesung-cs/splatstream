#ifndef VKGS_CORE_SEMAPHORE_H
#define VKGS_CORE_SEMAPHORE_H

#include <memory>

#include "volk.h"

namespace vkgs {
namespace core {

class Module;
class Command;

class Semaphore {
 public:
  Semaphore(std::shared_ptr<Module> module);
  ~Semaphore();

  VkSemaphore semaphore() const noexcept { return semaphore_; }
  auto value() const noexcept { return value_; }

  void Wait();
  void SignalBy(std::shared_ptr<Command> command, int64_t value);

 private:
  std::shared_ptr<Module> module_;

  VkSemaphore semaphore_ = VK_NULL_HANDLE;
  uint64_t value_ = 0;

  // Command buffer that waits on this semaphore.
  std::shared_ptr<Command> command_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_SEMAPHORE_H
