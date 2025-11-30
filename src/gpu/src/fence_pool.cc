#include "fence_pool.h"

#include <volk.h>

#include "vkgs/gpu/fence.h"

namespace vkgs {
namespace gpu {

FencePool::FencePool(VkDevice device) : device_(device) {}

FencePool::~FencePool() {
  for (auto fence : fences_) {
    vkDestroyFence(device_, fence, NULL);
  }
}

Fence FencePool::Allocate() {
  VkFence fence;
  if (fences_.empty()) {
    VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(device_, &fence_info, NULL, &fence);
  } else {
    fence = fences_.back();
    fences_.pop_back();
  }
  return Fence::Create(shared_from_this(), fence);
}

void FencePool::Free(VkFence fence) { fences_.push_back(fence); }

}  // namespace gpu
}  // namespace vkgs
