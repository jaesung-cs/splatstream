#include "task.h"

#include "fence.h"

namespace vkgs {
namespace core {

Task::Task(std::shared_ptr<Fence> fence, std::vector<std::shared_ptr<Object>> objects)
    : fence_(std::move(fence)), objects_(std::move(objects)) {}

Task::~Task() { Wait(); }

bool Task::IsDone() { return fence_->IsSignaled(); }

void Task::Wait() { fence_->Wait(); }

}  // namespace core
}  // namespace vkgs
