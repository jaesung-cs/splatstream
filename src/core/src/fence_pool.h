#ifndef VKGS_CORE_FENCE_POOL_H
#define VKGS_CORE_FENCE_POOL_H

#include <memory>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace core {

class Module;
class Fence;

class FencePool : public std::enable_shared_from_this<FencePool> {
 public:
  explicit FencePool(Module* module);
  ~FencePool();

  auto module() const noexcept { return module_; }

  std::shared_ptr<Fence> Allocate();
  void Free(VkFence fence);

 private:
  Module* module_;
  std::vector<VkFence> fences_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_FENCE_POOL_H
