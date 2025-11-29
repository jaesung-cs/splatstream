#ifndef VKGS_VIEWER_STORAGE_H
#define VKGS_VIEWER_STORAGE_H

#include <memory>

namespace vkgs {

namespace core {
class ScreenSplats;
}

namespace gpu {
class Image;
class Semaphore;
}  // namespace gpu

namespace viewer {

class Storage {
 public:
  Storage();
  ~Storage();

  void Update(uint32_t size, uint32_t width, uint32_t height);

  auto screen_splats() const noexcept { return screen_splats_; }

  // Color image
  auto image() const noexcept { return image_; }

  // Depth image
  auto depth_image() const noexcept { return depth_image_; }

  // Depth attachment
  auto depth() const noexcept { return depth_; }

  auto compute_semaphore() const noexcept { return compute_semaphore_; }
  auto graphics_semaphore() const noexcept { return graphics_semaphore_; }

 private:
  std::shared_ptr<core::ScreenSplats> screen_splats_;
  std::shared_ptr<gpu::Image> image_;
  std::shared_ptr<gpu::Image> depth_image_;
  std::shared_ptr<gpu::Image> depth_;
  std::shared_ptr<gpu::Semaphore> compute_semaphore_;
  std::shared_ptr<gpu::Semaphore> graphics_semaphore_;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_STORAGE_H
