#ifndef VKGS_GPU_FENCE_H
#define VKGS_GPU_FENCE_H

#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class FencePool;

class FenceImpl : public Object {
 public:
  FenceImpl(FencePool fence_pool, VkFence fence);
  ~FenceImpl();

  operator VkFence() const noexcept { return fence_; }

  bool IsSignaled();
  void Wait();

 private:
  FencePool fence_pool_;
  VkFence fence_ = VK_NULL_HANDLE;
};

class Fence : public SharedAccessor<Fence, FenceImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_FENCE_H
