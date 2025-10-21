#ifndef VKGS_GPU_FENCE_H
#define VKGS_GPU_FENCE_H

#include <memory>

#include "volk.h"

#include "export_api.h"

namespace vkgs {
namespace gpu {

class FencePool;

class VKGS_GPU_API Fence {
 public:
  Fence(VkDevice device, std::shared_ptr<FencePool> fence_pool, VkFence fence);
  ~Fence();

  operator VkFence() const noexcept { return fence_; }

  bool IsSignaled();
  void Wait();

 private:
  VkDevice device_;
  std::shared_ptr<FencePool> fence_pool_;
  VkFence fence_ = VK_NULL_HANDLE;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_FENCE_H
