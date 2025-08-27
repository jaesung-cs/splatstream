#include "task.h"

#include "fence.h"

namespace vkgs {
namespace core {

Task::Task(std::vector<std::shared_ptr<Semaphore>> wait_semaphores, std::shared_ptr<Command> command,
           std::vector<std::shared_ptr<Semaphore>> signal_semaphores, std::shared_ptr<Fence> fence)
    : wait_semaphores_(std::move(wait_semaphores)),
      command_(command),
      signal_semaphores_(std::move(signal_semaphores)),
      fence_(fence) {}

Task::~Task() { Wait(); }

bool Task::IsDone() { return fence_->IsSignaled(); }

void Task::Wait() { fence_->Wait(); }

}  // namespace core
}  // namespace vkgs
