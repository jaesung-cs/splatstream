#include "vkgs/core/rendering_task.h"

#include <cstring>

#include "vkgs/gpu/fence.h"
#include "vkgs/gpu/buffer.h"

namespace vkgs {
namespace core {

RenderingTask::RenderingTask(std::vector<std::shared_ptr<gpu::Fence>> fences, std::shared_ptr<gpu::Buffer> image_stage,
                             uint64_t size, uint8_t* dst)
    : fences_(std::move(fences)), image_stage_(image_stage), size_(size), dst_(dst) {}

RenderingTask::~RenderingTask() {}

void RenderingTask::Wait() {
  if (!fences_.empty()) {
    for (const auto& fence : fences_) {
      fence->Wait();
    }
    fences_.clear();

    std::memcpy(dst_, image_stage_->data<uint8_t>(), size_);
    image_stage_ = nullptr;
  }
}

}  // namespace core
}  // namespace vkgs
