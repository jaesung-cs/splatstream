#ifndef VKGS_CORE_SEMAPHORE_POOL_H
#define VKGS_CORE_SEMAPHORE_POOL_H

#include <memory>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace core {

class Module;
class Semaphore;

class SemaphorePool : public std::enable_shared_from_this<SemaphorePool> {
 public:
  SemaphorePool(std::shared_ptr<Module> module);
  ~SemaphorePool();

  std::shared_ptr<Semaphore> Allocate();
  void Free(VkSemaphore semaphore);

  auto module() const noexcept { return module_; }

 private:
  std::shared_ptr<Module> module_;

  std::vector<VkSemaphore> semaphores_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_SEMAPHORE_POOL_H
