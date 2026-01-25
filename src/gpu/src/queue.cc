#include "vkgs/gpu/queue.h"

#include "vkgs/gpu/details/command_pool.h"
#include "vkgs/gpu/details/command.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API QueueImpl {
 public:
  void __init__(VkDevice device, VkQueue queue, uint32_t family_index) {
    device_ = device;
    queue_ = queue;
    family_index_ = family_index;
    command_pool_ = CommandPool::Create(device_, family_index_);
  }

  operator VkQueue() const noexcept { return queue_; }
  operator uint32_t() const noexcept { return family_index_; }
  auto family_index() const noexcept { return family_index_; }

  Command AllocateCommandBuffer() { return command_pool_.Allocate(); }

 private:
  VkDevice device_;

  VkQueue queue_ = VK_NULL_HANDLE;
  uint32_t family_index_ = 0;

  CommandPool command_pool_;
};

Queue Queue::Create(VkDevice device, VkQueue queue, uint32_t family_index) {
  return Make<QueueImpl>(device, queue, family_index);
}

Queue::operator VkQueue() const { return impl_->operator VkQueue(); }
Queue::operator uint32_t() const { return impl_->operator uint32_t(); }
auto Queue::family_index() const { return impl_->family_index(); }

Command Queue::AllocateCommandBuffer() { return impl_->AllocateCommandBuffer(); }

}  // namespace gpu
}  // namespace vkgs
