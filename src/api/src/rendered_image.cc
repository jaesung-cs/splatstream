#include "vkgs/rendered_image.h"

#include "rendered_image_impl.h"

namespace vkgs {

RenderedImage::RenderedImage() : impl_(std::make_shared<Impl>()) {}

RenderedImage::~RenderedImage() {}

uint32_t RenderedImage::width() const { return impl_->width(); }
uint32_t RenderedImage::height() const { return impl_->height(); }

void RenderedImage::Wait() const { impl_->Wait(); }

}  // namespace vkgs
