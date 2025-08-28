#ifndef VKGS_CORE_QUEUE_H
#define VKGS_CORE_QUEUE_H

#include "volk.h"

namespace vkgs {
namespace core {

class Queue {
 public:
  Queue(uint32_t queue_family_index, VkQueue queue) : queue_family_index_(queue_family_index), queue_(queue) {}

  ~Queue() = default;

  auto queue_family_index() const noexcept { return queue_family_index_; }
  auto queue() const noexcept { return queue_; }

 private:
  uint32_t queue_family_index_;
  VkQueue queue_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_QUEUE_H
