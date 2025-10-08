#ifndef VKGS_CORE_FENCE_POOL_H
#define VKGS_CORE_FENCE_POOL_H

#include <memory>
#include <vector>

#include "volk.h"

namespace vkgs {
namespace core {

class Fence;

class FencePool : public std::enable_shared_from_this<FencePool> {
 public:
  explicit FencePool(VkDevice device);
  ~FencePool();

  std::shared_ptr<Fence> Allocate();
  void Free(VkFence fence);

 private:
  VkDevice device_;
  std::vector<VkFence> fences_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_FENCE_POOL_H
