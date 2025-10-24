#include "vkgs/rendered_image.h"

#include "vkgs/core/rendered_image.h"

namespace vkgs {

RenderedImage::RenderedImage(std::shared_ptr<core::RenderedImage> rendered_image) : rendered_image_(rendered_image) {}

RenderedImage::~RenderedImage() {}

uint32_t RenderedImage::width() const { return rendered_image_->width(); }
uint32_t RenderedImage::height() const { return rendered_image_->height(); }

void RenderedImage::Wait() const { rendered_image_->Wait(); }

}  // namespace vkgs
