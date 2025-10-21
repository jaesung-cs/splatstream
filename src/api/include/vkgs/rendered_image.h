#ifndef VKGS_RENDERED_IMAGE_H
#define VKGS_RENDERED_IMAGE_H

#include <memory>
#include <vector>

#include "vkgs/export_api.h"

namespace vkgs {

class VKGS_API RenderedImage {
 public:
  RenderedImage();
  ~RenderedImage();

  uint32_t width() const;
  uint32_t height() const;

  void Wait() const;

  auto* impl() noexcept { return impl_.get(); }

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERED_IMAGE_H
