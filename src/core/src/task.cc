#include "task.h"

#include "fence.h"

namespace vkgs {
namespace core {

Task::Task(std::shared_ptr<Command> command, std::shared_ptr<Fence> fence,
           const std::vector<std::shared_ptr<Buffer>>& buffers)
    : command_(command), fence_(fence), buffers_(buffers) {}

Task::~Task() { Wait(); }

bool Task::IsDone() { return fence_->IsSignaled(); }

void Task::Wait() { fence_->Wait(); }

}  // namespace core
}  // namespace vkgs
