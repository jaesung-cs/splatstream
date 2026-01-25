#ifndef VKGS_GPU_DETAILS_FENCE_H
#define VKGS_GPU_DETAILS_FENCE_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"

namespace vkgs {
namespace gpu {

class FencePool;

class FenceImpl;
class Fence : public Handle<Fence, FenceImpl> {
 public:
  static Fence Create(FencePool fence_pool, VkFence fence);

  operator VkFence() const noexcept;
  bool IsSignaled();
  void Wait();
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_DETAILS_FENCE_H
