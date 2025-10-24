#ifndef VKGS_RENDERED_IMAGE_H
#define VKGS_RENDERED_IMAGE_H

#include <memory>
#include <vector>

#include "vkgs/export_api.h"

namespace vkgs {
namespace core {
class RenderedImage;
}

class VKGS_API RenderedImage {
 public:
  explicit RenderedImage(std::shared_ptr<core::RenderedImage> rendered_image);
  ~RenderedImage();

  uint32_t width() const;
  uint32_t height() const;

  void Wait() const;

 private:
  std::shared_ptr<core::RenderedImage> rendered_image_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERED_IMAGE_H
