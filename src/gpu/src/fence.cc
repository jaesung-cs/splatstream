#include "fence.h"

#include <volk.h>

#include "fence_pool.h"

namespace vkgs {
namespace gpu {

Fence::Fence(VkDevice device, std::shared_ptr<FencePool> fence_pool, VkFence fence)
    : device_(device), fence_pool_(fence_pool), fence_(fence) {}

Fence::~Fence() {
  Wait();
  vkResetFences(device_, 1, &fence_);
  fence_pool_->Free(fence_);
}

bool Fence::IsSignaled() {
  VkResult result = vkWaitForFences(device_, 1, &fence_, VK_TRUE, 0);
  return result == VK_SUCCESS;
}

void Fence::Wait() { vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX); }

}  // namespace gpu
}  // namespace vkgs
