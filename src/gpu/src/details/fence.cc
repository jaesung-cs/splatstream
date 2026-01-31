#include "details/fence.h"

#include "volk.h"

#include "vkgs/gpu/object.h"

#include "details/fence_pool.h"

namespace vkgs {
namespace gpu {

class FenceImpl : public Object {
 public:
  void __init__(FencePool fence_pool, VkFence fence) {
    fence_pool_ = fence_pool;
    fence_ = fence;
  }

  void __del__() {
    Wait();
    vkResetFences(device_, 1, &fence_);
    fence_pool_.Free(fence_);
  }

  operator VkFence() const noexcept { return fence_; }

  bool IsSignaled() {
    VkResult result = vkWaitForFences(device_, 1, &fence_, VK_TRUE, 0);
    return result == VK_SUCCESS;
  }

  void Wait() { vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX); }

 private:
  FencePool fence_pool_;
  VkFence fence_ = VK_NULL_HANDLE;
};

Fence Fence::Create(FencePool fence_pool, VkFence fence) { return Make<FenceImpl>(fence_pool, fence); }

Fence::operator VkFence() const noexcept { return *impl_; }
bool Fence::IsSignaled() { return impl_->IsSignaled(); }
void Fence::Wait() { impl_->Wait(); }

}  // namespace gpu
}  // namespace vkgs
