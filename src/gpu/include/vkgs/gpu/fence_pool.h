#ifndef VKGS_GPU_FENCE_POOL_H
#define VKGS_GPU_FENCE_POOL_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"

namespace vkgs {
namespace gpu {

class Fence;

class FencePoolImpl : public std::enable_shared_from_this<FencePoolImpl> {
 public:
  explicit FencePoolImpl(VkDevice device);
  ~FencePoolImpl();

  Fence Allocate();
  void Free(VkFence fence);

 private:
  VkDevice device_;
  std::vector<VkFence> fences_;
};

class FencePool : public SharedAccessor<FencePool, FencePoolImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_FENCE_POOL_H
