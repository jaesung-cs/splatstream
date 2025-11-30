#ifndef VKGS_VIEWER_STORAGE_H
#define VKGS_VIEWER_STORAGE_H

#include <memory>

#include "imgui_texture.h"

#include "vkgs/gpu/sampler.h"
#include "vkgs/gpu/image.h"
#include "vkgs/gpu/semaphore.h"

namespace vkgs {

namespace core {
class ScreenSplats;
}

namespace viewer {

class Storage {
 public:
  Storage();
  ~Storage();

  void Update(uint32_t size, uint32_t width, uint32_t height);
  void Clear();

  auto screen_splats() const noexcept { return screen_splats_; }

  // Texture for ImGui
  auto texture() const noexcept { return texture_; }

  // Final image
  auto image() const noexcept { return image_; }

  // Intermediate color image
  auto image16() const noexcept { return image16_; }

  // Depth image
  auto depth_image() const noexcept { return depth_image_; }

  // Depth attachment
  auto depth() const noexcept { return depth_; }

  auto compute_semaphore() const noexcept { return compute_semaphore_; }
  auto graphics_semaphore() const noexcept { return graphics_semaphore_; }

 private:
  std::shared_ptr<core::ScreenSplats> screen_splats_;
  gpu::Image image_;
  gpu::Image image16_;
  gpu::Image depth_image_;
  gpu::Image depth_;
  gpu::Semaphore compute_semaphore_;
  gpu::Semaphore graphics_semaphore_;

  // For ImGui texture
  gpu::Sampler sampler_;
  ImGuiTexture texture_;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_STORAGE_H
