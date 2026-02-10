#include "details/timer_pool.h"

#include "volk.h"

namespace vkgs {
namespace gpu {

class TimerPoolImpl {
 public:
  void __init__(VkDevice device) {
    device_ = device;

    VkQueryPoolCreateInfo query_pool_info = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = kQueryCount;
    vkCreateQueryPool(device_, &query_pool_info, NULL, &query_pool_);
  }

  void __del__() { vkDestroyQueryPool(device_, query_pool_, NULL); }

  TimerAllocation Allocate(uint32_t size) {
    if (index_ + size > kQueryCount) {
      // TODO: create a new query pool instead of reusing the same one
      index_ = 0;
    }

    TimerAllocation allocation;
    allocation.query_pool = query_pool_;
    allocation.start = index_;
    allocation.size = size;

    index_ += size;
    return allocation;
  }

  void Free(const TimerAllocation& allocation) {
    // TODO
  }

 private:
  VkDevice device_;

  static constexpr uint32_t kQueryCount = 2048;
  VkQueryPool query_pool_ = VK_NULL_HANDLE;
  uint32_t index_ = 0;
};

TimerPool TimerPool::Create(VkDevice device) { return Make<TimerPoolImpl>(device); }

TimerAllocation TimerPool::Allocate(uint32_t size) { return impl_->Allocate(size); }
void TimerPool::Free(const TimerAllocation& allocation) { impl_->Free(allocation); }

}  // namespace gpu
}  // namespace vkgs
