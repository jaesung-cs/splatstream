#include "vkgs/rendering_task.h"

#include "vkgs/core/rendering_task.h"
#include "vkgs/core/draw_result.h"

namespace vkgs {

class RenderingTask::Impl {
 public:
  Impl(core::RenderingTask task) : task_(task) {}
  ~Impl() = default;

  void Wait() {
    task_.Wait();

    auto result = task_.draw_result();
    result_ = {
        .compute_timestamp = result.compute_timestamp,
        .graphics_timestamp = result.graphics_timestamp,
        .transfer_timestamp = result.transfer_timestamp,
    };
  }

  const DrawResult& draw_result() const { return result_; }

 private:
  core::RenderingTask task_;
  DrawResult result_ = {};
};

RenderingTask::RenderingTask(core::RenderingTask task) : impl_(std::make_shared<Impl>(task)) {}

RenderingTask::~RenderingTask() = default;

void RenderingTask::Wait() { impl_->Wait(); }

const DrawResult& RenderingTask::draw_result() const { return impl_->draw_result(); }

}  // namespace vkgs
