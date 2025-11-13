#ifndef VKGS_GPU_FENCE_POOL_H
#define VKGS_GPU_FENCE_POOL_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

namespace vkgs {
namespace gpu {

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

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_FENCE_POOL_H
