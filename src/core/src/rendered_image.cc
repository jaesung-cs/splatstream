#include "vkgs/core/rendered_image.h"

namespace vkgs {
namespace core {

RenderedImage::RenderedImage(uint32_t width, uint32_t height, std::vector<uint8_t> data)
    : width_(width), height_(height), data_(std::move(data)) {}

RenderedImage::~RenderedImage() {}

}  // namespace core
}  // namespace vkgs
