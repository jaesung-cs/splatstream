#include "vkgs/gpu/fence.h"

#include <volk.h>

#include "vkgs/gpu/device.h"

#include "fence_pool.h"

namespace vkgs {
namespace gpu {

FenceImpl::FenceImpl(std::shared_ptr<FencePool> fence_pool, VkFence fence) : fence_pool_(fence_pool), fence_(fence) {}

FenceImpl::~FenceImpl() {
  Wait();
  vkResetFences(*device_, 1, &fence_);
  fence_pool_->Free(fence_);
}

bool FenceImpl::IsSignaled() {
  VkResult result = vkWaitForFences(*device_, 1, &fence_, VK_TRUE, 0);
  return result == VK_SUCCESS;
}

void FenceImpl::Wait() { vkWaitForFences(*device_, 1, &fence_, VK_TRUE, UINT64_MAX); }

template class SharedAccessor<Fence, FenceImpl>;

}  // namespace gpu
}  // namespace vkgs
