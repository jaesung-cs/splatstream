#ifndef VKGS_GPU_DETAILS_FENCE_POOL_H
#define VKGS_GPU_DETAILS_FENCE_POOL_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace gpu {

class Fence;

class FencePoolImpl;
class FencePool : public Handle<FencePool, FencePoolImpl> {
 public:
  static FencePool Create(VkDevice device);

  Fence Allocate();
  void Free(VkFence fence);
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_FENCE_POOL_H
