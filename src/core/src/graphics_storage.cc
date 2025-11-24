#include "graphics_storage.h"

#include "vkgs/gpu/image.h"

namespace vkgs {
namespace core {

GraphicsStorage::GraphicsStorage() {}

GraphicsStorage::~GraphicsStorage() {}

void GraphicsStorage::Update(uint32_t width, uint32_t height) {
  if (width_ != width || height_ != height) {
    image_ = gpu::Image::Create(VK_FORMAT_R16G16B16A16_SFLOAT, width, height,
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    image_u8_ = gpu::Image::Create(
        VK_FORMAT_R8G8B8A8_UNORM, width, height,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    width_ = width;
    height_ = height;
  }
}

}  // namespace core
}  // namespace vkgs