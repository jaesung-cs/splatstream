#include "rendered_image_impl.h"

namespace vkgs {

RenderedImage::Impl::Impl() = default;

RenderedImage::Impl::~Impl() = default;

uint32_t RenderedImage::Impl::width() const { return rendered_image_->width(); }
uint32_t RenderedImage::Impl::height() const { return rendered_image_->height(); }
const std::vector<uint8_t>& RenderedImage::Impl::data() const { return rendered_image_->data(); }

}  // namespace vkgs
