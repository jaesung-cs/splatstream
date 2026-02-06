#include "details/timer_pool.h"

#include "volk.h"

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/timer.h"

namespace vkgs {
namespace gpu {

class TimerPoolImpl : public EnableHandleFromThis<TimerPool, TimerPoolImpl> {
 public:
  void __init__(Device device) {
    device_ = device;

    VkQueryPoolCreateInfo query_pool_info = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = kQueryCount;
    vkCreateQueryPool(device_, &query_pool_info, NULL, &query_pool_);
  }

  void __del__() { vkDestroyQueryPool(device_, query_pool_, NULL); }

  Timer Allocate(uint32_t size) {
    if (index_ + size > kQueryCount) index_ = 0;
    auto timer = Timer::Create(HandleFromThis(), query_pool_, index_, size);
    index_ += size;
    return timer;
  }

  void Free(VkQueryPool query_pool, uint32_t start, uint32_t size) {
    // TODO
  }

 private:
  Device::Weak device_;

  static constexpr uint32_t kQueryCount = 2048;
  VkQueryPool query_pool_ = VK_NULL_HANDLE;
  uint32_t index_ = 0;
};

TimerPool TimerPool::Create(Device device) { return Make<TimerPoolImpl>(device); }

Timer TimerPool::Allocate(uint32_t size) { return impl_->Allocate(size); }
void TimerPool::Free(VkQueryPool query_pool, uint32_t start, uint32_t size) { impl_->Free(query_pool, start, size); }

}  // namespace gpu
}  // namespace vkgs
