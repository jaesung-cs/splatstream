#include "storage.h"

#include "vkgs/core/screen_splats.h"
#include "vkgs/core/stats.h"
#include "vkgs/gpu/gpu.h"
#include "vkgs/gpu/device.h"
#include "vkgs/gpu/image.h"

namespace vkgs {
namespace viewer {

Storage::Storage() = default;

Storage::~Storage() = default;

void Storage::Update(uint32_t size, uint32_t width, uint32_t height) {
  auto device = gpu::GetDevice();

  if (!screen_splats_) screen_splats_ = core::ScreenSplats::Create();
  screen_splats_->Update(size);

  if (!visible_point_count_stage_)
    visible_point_count_stage_ = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(uint32_t), true);
  if (!stats_stage_) stats_stage_ = gpu::Buffer::Create(VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(core::Stats), true);

  if (!compute_semaphore_) compute_semaphore_ = device->AllocateSemaphore();
  if (!graphics_semaphore_) graphics_semaphore_ = device->AllocateSemaphore();

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
  screen_splats_.reset();
  visible_point_count_stage_.reset();
  stats_stage_.reset();
  image16_.reset();
  depth_image_.reset();
  depth_.reset();
  compute_semaphore_.reset();
  graphics_semaphore_.reset();
  task_.reset();
}

}  // namespace viewer
}  // namespace vkgs
