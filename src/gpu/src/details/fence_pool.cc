#include "vkgs/gpu/details/fence_pool.h"

#include <volk.h>

#include "vkgs/gpu/details/fence.h"

namespace vkgs {
namespace gpu {

FencePoolImpl::FencePoolImpl(VkDevice device) : device_(device) {}

FencePoolImpl::~FencePoolImpl() {
  for (auto fence : fences_) {
    vkDestroyFence(device_, fence, NULL);
  }
}

Fence FencePoolImpl::Allocate() {
  VkFence fence;
  if (fences_.empty()) {
    VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    vkCreateFence(device_, &fence_info, NULL, &fence);
  } else {
    fence = fences_.back();
    fences_.pop_back();
  }
  return Fence::Create(FencePool::FromPtr(shared_from_this()), fence);
}

void FencePoolImpl::Free(VkFence fence) { fences_.push_back(fence); }

}  // namespace gpu
}  // namespace vkgs
