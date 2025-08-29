#include "task.h"

#include "fence.h"

namespace vkgs {
namespace core {

Task::Task(std::shared_ptr<Command> command, std::vector<std::shared_ptr<Semaphore>> semaphores,
           std::shared_ptr<Fence> fence)
    : semaphores_(std::move(semaphores)), command_(command), fence_(fence) {}

Task::~Task() { Wait(); }

bool Task::IsDone() { return fence_->IsSignaled(); }

void Task::Wait() { fence_->Wait(); }

}  // namespace core
}  // namespace vkgs
