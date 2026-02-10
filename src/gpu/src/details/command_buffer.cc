#include "details/command_buffer.h"

#include "vkgs/gpu/object.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/queue.h"

#include "details/command_pool.h"

namespace vkgs {
namespace gpu {

class CommandBufferImpl : public Object {
 public:
  void __init__(QueueType queue_type) {
    command_pool_ = device_.queue(queue_type).command_pool();
    cb_ = command_pool_.Allocate();
  }

  void __del__() { command_pool_.Free(cb_); }

  operator VkCommandBuffer() const noexcept { return cb_; }

 private:
  CommandPool command_pool_;
  VkCommandBuffer cb_ = VK_NULL_HANDLE;
};

CommandBuffer CommandBuffer::Create(QueueType queue_type) { return Make<CommandBufferImpl>(queue_type); }

CommandBuffer::operator VkCommandBuffer() const { return *impl_; }

}  // namespace gpu
}  // namespace vkgs
