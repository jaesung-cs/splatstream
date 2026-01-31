#include "details/fence_pool.h"

#include <vector>

#include "volk.h"

#include "details/fence.h"

namespace vkgs {
namespace gpu {

class FencePoolImpl : public EnableHandleFromThis<FencePool, FencePoolImpl> {
 public:
  void __init__(VkDevice device) { device_ = device; }

  void __del__() {
    for (auto fence : fences_) {
      vkDestroyFence(device_, fence, NULL);
    }
  }

  Fence Allocate() {
    VkFence fence;
    if (fences_.empty()) {
      VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
      vkCreateFence(device_, &fence_info, NULL, &fence);
    } else {
      fence = fences_.back();
      fences_.pop_back();
    }
    return Fence::Create(HandleFromThis(), fence);
  }

  void Free(VkFence fence) { fences_.push_back(fence); }

 private:
  VkDevice device_;
  std::vector<VkFence> fences_;
};

FencePool FencePool::Create(VkDevice device) { return Make<FencePoolImpl>(device); }

Fence FencePool::Allocate() { return impl_->Allocate(); }
void FencePool::Free(VkFence fence) { impl_->Free(fence); }

}  // namespace gpu
}  // namespace vkgs
