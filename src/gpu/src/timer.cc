#include "vkgs/gpu/timer.h"

#include <stdexcept>

#include <volk.h>

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {

std::shared_ptr<Timer> Timer::Create(uint32_t size) { return std::make_shared<Timer>(size); }

Timer::Timer(uint32_t size) : size_(size) {
  VkQueryPoolCreateInfo query_pool_info = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
  query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
  query_pool_info.queryCount = size;
  vkCreateQueryPool(*device_, &query_pool_info, NULL, &query_pool_);
  vkResetQueryPool(*device_, query_pool_, 0, size);
}

Timer::~Timer() { vkDestroyQueryPool(*device_, query_pool_, NULL); }

void Timer::Record(VkCommandBuffer cb, VkPipelineStageFlags2 stage) {
  if (counter_ >= size_) {
    throw std::runtime_error("Timer: counter out of range");
  }

  vkCmdWriteTimestamp2(cb, stage, query_pool_, counter_);
  counter_++;
}

std::vector<uint64_t> Timer::GetTimestamps() const {
  std::vector<uint64_t> timestamps(counter_);
  vkGetQueryPoolResults(*device_, query_pool_, 0, counter_, counter_ * sizeof(uint64_t), timestamps.data(),
                        sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
  return timestamps;
}

}  // namespace gpu
}  // namespace vkgs
