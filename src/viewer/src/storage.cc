#include "storage.h"

#include "vkgs/core/screen_splats.h"
#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/image.h"
#include "vkgs/gpu/sampler.h"

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

  if (!sampler_) sampler_ = gpu::Sampler::Create();

  if (!image_ || image_->width() != width || image_->height() != height) {
    image_ = gpu::Image::Create(VK_FORMAT_B8G8R8A8_UNORM, width, height,
                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    texture_ = ImGuiTexture::Create(*sampler_, image_->image_view());
  }
  if (!image16_ || image16_->width() != width || image16_->height() != height) {
    image16_ = gpu::Image::Create(VK_FORMAT_R16G16B16A16_SFLOAT, width, height,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                      VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
  }
  if (!depth_image_ || depth_image_->width() != width || depth_image_->height() != height) {
    depth_image_ = gpu::Image::Create(VK_FORMAT_R16G16_SFLOAT, width, height,
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
  }
  if (!depth_ || depth_->width() != width || depth_->height() != height) {
    depth_ = gpu::Image::Create(VK_FORMAT_D32_SFLOAT, width, height,
                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
  }
}

void Storage::Clear() {
  screen_splats_ = nullptr;
  image_ = nullptr;
  image16_ = nullptr;
  depth_image_ = nullptr;
  depth_ = nullptr;
  compute_semaphore_ = nullptr;
  graphics_semaphore_ = nullptr;
  sampler_ = nullptr;
  texture_ = nullptr;
}

}  // namespace viewer
}  // namespace vkgs
