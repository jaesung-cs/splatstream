#include "vkgs/core/rendered_image.h"

#include "vkgs/gpu/task.h"

namespace vkgs {
namespace core {

RenderedImage::RenderedImage(uint32_t width, uint32_t height, std::shared_ptr<gpu::Task> task)
    : width_(width), height_(height), task_(task) {}

RenderedImage::~RenderedImage() {}

void RenderedImage::Wait() {
  if (task_) {
    task_->Wait();
    task_ = nullptr;
  }
}

}  // namespace core
}  // namespace vkgs
