#include "vkgs/core/rendered_image.h"

#include <algorithm>

#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/fence.h"

namespace vkgs {
namespace core {

RenderedImage::RenderedImage(uint32_t width, uint32_t height, std::shared_ptr<gpu::Fence> fence,
                             std::shared_ptr<gpu::Buffer> image_buffer, uint8_t* dst)
    : width_(width), height_(height), fence_(fence), image_buffer_(image_buffer), dst_(dst) {}

RenderedImage::~RenderedImage() {}

void RenderedImage::Wait() {
  if (fence_) {
    fence_->Wait();

    const auto* image_buffer_ptr = image_buffer_->data<float>();

    for (int i = 0; i < width_ * height_ * 4; ++i) {
      dst_[i] = static_cast<uint8_t>(std::clamp(image_buffer_ptr[i], 0.f, 1.f) * 255.f);
    }

    fence_ = nullptr;
    image_buffer_ = nullptr;
  }
}

}  // namespace core
}  // namespace vkgs
