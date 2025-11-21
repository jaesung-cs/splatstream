#include "vkgs/viewer/storage.h"

#include "vkgs/core/screen_splats.h"
#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/image.h"

namespace vkgs {
namespace viewer {

Storage::Storage() = default;

Storage::~Storage() = default;

void Storage::Update(uint32_t size, uint32_t width, uint32_t height) {
  auto device = gpu::GetDevice();

  if (!screen_splats_) screen_splats_ = std::make_shared<core::ScreenSplats>();
  screen_splats_->Update(size);

  if (!compute_semaphore_) compute_semaphore_ = device->AllocateSemaphore();
  if (!graphics_semaphore_) graphics_semaphore_ = device->AllocateSemaphore();

  if (!image_ || image_->width() != width || image_->height() != height) {
    image_ = gpu::Image::Create(VK_FORMAT_R16G16B16A16_SFLOAT, width, height,
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
  }
  if (!depth_image_ || depth_image_->width() != width || depth_image_->height() != height) {
    depth_image_ =
        gpu::Image::Create(VK_FORMAT_D32_SFLOAT, width, height,
                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
  }
}

}  // namespace viewer
}  // namespace vkgs
