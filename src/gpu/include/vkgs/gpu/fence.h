#ifndef VKGS_GPU_FENCE_H
#define VKGS_GPU_FENCE_H

#include <memory>

#include <vulkan/vulkan.h>

#include "object.h"

namespace vkgs {
namespace gpu {

class FencePool;

class FenceImpl : public Object {
 public:
  FenceImpl(std::shared_ptr<FencePool> fence_pool, VkFence fence);
  ~FenceImpl();

  operator VkFence() const noexcept { return fence_; }

  bool IsSignaled();
  void Wait();

 private:
  std::shared_ptr<FencePool> fence_pool_;
  VkFence fence_ = VK_NULL_HANDLE;
};

class Fence : public SharedAccessor<Fence, FenceImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_FENCE_H
