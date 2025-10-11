#ifndef VKGS_CORE_RENDERED_IMAGE_H
#define VKGS_CORE_RENDERED_IMAGE_H

#include <vector>

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class VKGS_CORE_API RenderedImage {
 public:
  RenderedImage(uint32_t width, uint32_t height, std::vector<uint8_t> data);
  ~RenderedImage();

  uint32_t width() const noexcept { return width_; }
  uint32_t height() const noexcept { return height_; }
  const auto& data() const noexcept { return data_; }

 private:
  uint32_t width_;
  uint32_t height_;
  std::vector<uint8_t> data_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERED_IMAGE_H
