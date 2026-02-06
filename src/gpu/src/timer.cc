#include "vkgs/gpu/timer.h"

#include <stdexcept>
#include <iostream>

#include "volk.h"

#include "vkgs/gpu/object.h"

#include "details/timer_pool.h"

namespace vkgs {
namespace gpu {

class TimerImpl : public Object {
 public:
  void __init__(TimerPool pool, VkQueryPool query_pool, uint32_t start, uint32_t size) {
    pool_ = pool;
    query_pool_ = query_pool;
    start_ = start;
    size_ = size;
  }

  void __del__() { pool_.Free(query_pool_, start_, size_); }

  void Record(VkCommandBuffer cb, VkPipelineStageFlags2 stage) {
    if (counter_ >= size_) {
      throw std::runtime_error("Timer: counter out of range");
    }

    if (counter_ == 0) {
      std::cout << "Query reset: " << start_ << ", " << size_ << std::endl;
      vkCmdResetQueryPool(cb, query_pool_, start_, size_);
    }
    vkCmdWriteTimestamp2(cb, stage, query_pool_, counter_);
    counter_++;
  }

  std::vector<uint64_t> GetTimestamps() const {
    std::vector<uint64_t> timestamps(counter_);
    vkGetQueryPoolResults(device_, query_pool_, start_, counter_, counter_ * sizeof(uint64_t), timestamps.data(),
                          sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    return timestamps;
  }

 private:
  TimerPool pool_;

  VkQueryPool query_pool_ = VK_NULL_HANDLE;
  uint32_t start_ = 0;
  uint32_t size_ = 0;

  uint32_t counter_ = 0;
};

Timer Timer::Create(TimerPool pool, VkQueryPool query_pool, uint32_t start, uint32_t size) {
  return Make<TimerImpl>(pool, query_pool, start, size);
}

void Timer::Record(VkCommandBuffer cb, VkPipelineStageFlags2 stage) { impl_->Record(cb, stage); }
std::vector<uint64_t> Timer::GetTimestamps() const { return impl_->GetTimestamps(); }

}  // namespace gpu
}  // namespace vkgs
