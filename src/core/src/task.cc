#include "task.h"

#include "fence.h"

namespace vkgs {
namespace core {

Task::Task(std::shared_ptr<Command> command, std::vector<std::shared_ptr<Semaphore>> semaphores,
           std::shared_ptr<Fence> fence)
    : command_(command), semaphores_(std::move(semaphores)), fence_(fence) {}

Task::~Task() { Wait(); }

bool Task::IsDone() { return fence_->IsSignaled(); }

void Task::Wait() { fence_->Wait(); }

}  // namespace core
}  // namespace vkgs
