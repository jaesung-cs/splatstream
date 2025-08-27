#include "fence.h"

#include "vkgs/core/module.h"

#include "fence_pool.h"

namespace vkgs {
namespace core {

Fence::Fence(std::shared_ptr<FencePool> fence_pool, VkFence fence) : fence_pool_(fence_pool), fence_(fence) {}

Fence::~Fence() {
  Wait();
  vkResetFences(fence_pool_->module()->device(), 1, &fence_);
  fence_pool_->Free(fence_);
}

bool Fence::IsSignaled() {
  VkResult result = vkWaitForFences(fence_pool_->module()->device(), 1, &fence_, VK_TRUE, UINT64_MAX);
  return result == VK_SUCCESS;
}

void Fence::Wait() { vkWaitForFences(fence_pool_->module()->device(), 1, &fence_, VK_TRUE, UINT64_MAX); }

}  // namespace core
}  // namespace vkgs
