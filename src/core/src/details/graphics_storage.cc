#include "details/graphics_storage.h"

#include "vkgs/gpu/image.h"

namespace vkgs {
namespace core {

class GraphicsStorageImpl {
 public:
  auto image() const noexcept { return image_; }
  auto image_u8() const noexcept { return image_u8_; }

  void Update(uint32_t width, uint32_t height) {
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

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  // Variable
  gpu::Image image_;     // (H, W, 4) float32
  gpu::Image image_u8_;  // (H, W, 4), UNORM
};

GraphicsStorage GraphicsStorage::Create() { return Make<GraphicsStorageImpl>(); }

gpu::Image GraphicsStorage::image() const { return impl_->image(); }
gpu::Image GraphicsStorage::image_u8() const { return impl_->image_u8(); }

void GraphicsStorage::Update(uint32_t width, uint32_t height) { impl_->Update(width, height); }

}  // namespace core
}  // namespace vkgs
