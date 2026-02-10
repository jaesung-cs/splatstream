#include "vkgs/gpu/timer.h"

#include <stdexcept>

#include "volk.h"

#include "vkgs/gpu/object.h"

#include "details/timer_pool.h"

namespace vkgs {
namespace gpu {

class TimerImpl : public Object {
 public:
  void __init__(uint32_t size) {
    pool_ = device_.timer_pool();
    allocation_ = pool_.Allocate(size);
  }

  void __del__() { pool_.Free(allocation_); }

  void Record(VkCommandBuffer cb, VkPipelineStageFlags2 stage) {
    if (counter_ >= allocation_.size) {
      throw std::runtime_error("Timer: counter out of range");
    }

    if (counter_ == 0) {
      vkCmdResetQueryPool(cb, allocation_.query_pool, allocation_.start, allocation_.size);
    }
    vkCmdWriteTimestamp2(cb, stage, allocation_.query_pool, allocation_.start + counter_);
    counter_++;
  }

  std::vector<uint64_t> GetTimestamps() const {
    std::vector<uint64_t> timestamps(counter_);
    if (counter_ > 0) {
      vkGetQueryPoolResults(device_, allocation_.query_pool, allocation_.start, counter_, counter_ * sizeof(uint64_t),
                            timestamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    }
    return timestamps;
  }

 private:
  TimerPool pool_;
  TimerAllocation allocation_;

  uint32_t counter_ = 0;
};

Timer Timer::Create(uint32_t size) { return Make<TimerImpl>(size); }

void Timer::Record(VkCommandBuffer cb, VkPipelineStageFlags2 stage) { impl_->Record(cb, stage); }
std::vector<uint64_t> Timer::GetTimestamps() const { return impl_->GetTimestamps(); }

}  // namespace gpu
}  // namespace vkgs
