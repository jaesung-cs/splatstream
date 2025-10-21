#include "rendered_image_impl.h"

namespace vkgs {

RenderedImage::Impl::Impl() = default;

RenderedImage::Impl::~Impl() = default;

uint32_t RenderedImage::Impl::width() const { return rendered_image_->width(); }
uint32_t RenderedImage::Impl::height() const { return rendered_image_->height(); }

void RenderedImage::Impl::Wait() const { rendered_image_->Wait(); }

}  // namespace vkgs
