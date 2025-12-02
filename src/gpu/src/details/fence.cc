#include "vkgs/gpu/details/fence.h"

#include <volk.h>

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/details/fence_pool.h"

namespace vkgs {
namespace gpu {

FenceImpl::FenceImpl(FencePool fence_pool, VkFence fence) : fence_pool_(fence_pool), fence_(fence) {}

FenceImpl::~FenceImpl() {
  Wait();
  vkResetFences(device_, 1, &fence_);
  fence_pool_->Free(fence_);
}

bool FenceImpl::IsSignaled() {
  VkResult result = vkWaitForFences(device_, 1, &fence_, VK_TRUE, 0);
  return result == VK_SUCCESS;
}

void FenceImpl::Wait() { vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX); }

}  // namespace gpu
}  // namespace vkgs
