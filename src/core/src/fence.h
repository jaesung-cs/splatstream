#ifndef VKGS_CORE_FENCE_H
#define VKGS_CORE_FENCE_H

#include <memory>

#include "volk.h"

namespace vkgs {
namespace core {

class FencePool;

class Fence {
 public:
  Fence(VkDevice device, std::shared_ptr<FencePool> fence_pool, VkFence fence);
  ~Fence();

  VkFence fence() const noexcept { return fence_; }

  bool IsSignaled();
  void Wait();

 private:
  VkDevice device_;
  std::shared_ptr<FencePool> fence_pool_;
  VkFence fence_ = VK_NULL_HANDLE;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_FENCE_H
