#ifndef VKGS_RENDERED_IMAGE_IMPL_H
#define VKGS_RENDERED_IMAGE_IMPL_H

#include "vkgs/rendered_image.h"

#include "vkgs/core/rendered_image.h"

namespace vkgs {

class RenderedImage::Impl {
 public:
  Impl();
  ~Impl();

  void SetRenderedImage(std::shared_ptr<core::RenderedImage> rendered_image) { rendered_image_ = rendered_image; }

  uint32_t width() const;
  uint32_t height() const;
  const std::vector<uint8_t>& data() const;

 private:
  std::shared_ptr<core::RenderedImage> rendered_image_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERED_IMAGE_IMPL_H
