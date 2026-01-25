#include "vkgs/core/rendering_task.h"

#include "vkgs/gpu/queue_task.h"
#include "vkgs/core/draw_result.h"

namespace vkgs {
namespace core {

class RenderingTaskImpl {
 public:
  void SetTask(gpu::QueueTask task) { task_ = task; }
  void SetDrawResult(const DrawResult& result) { result_ = result; }

  const auto& draw_result() const noexcept { return result_; }

  void Wait() {
    if (task_) {
      task_.Wait();
      task_.reset();
    }
  }

 private:
  gpu::QueueTask task_;
  DrawResult result_ = {};
};

RenderingTask RenderingTask::Create() { return Make<RenderingTaskImpl>(); }

void RenderingTask::SetTask(gpu::QueueTask task) { impl_->SetTask(task); }
void RenderingTask::SetDrawResult(const DrawResult& result) { impl_->SetDrawResult(result); }
const DrawResult& RenderingTask::draw_result() const { return impl_->draw_result(); }
void RenderingTask::Wait() { impl_->Wait(); }

}  // namespace core
}  // namespace vkgs
