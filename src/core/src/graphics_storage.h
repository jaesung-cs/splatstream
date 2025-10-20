#ifndef VKGS_CORE_GRAPHICS_STORAGE_H
#define VKGS_CORE_GRAPHICS_STORAGE_H

#include <memory>
#include <cstdint>

namespace vkgs {
namespace gpu {

class Device;
class Image;

}  // namespace gpu

namespace core {

class GraphicsStorage {
 public:
  GraphicsStorage(std::shared_ptr<gpu::Device> device);
  ~GraphicsStorage();

  auto image() const noexcept { return image_; }

  // TODO: enlarge when needed.
  void Update(uint32_t width, uint32_t height);

 private:
  std::shared_ptr<gpu::Device> device_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  // Variable
  std::shared_ptr<gpu::Image> image_;  // (H, W, 4) float32
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_GRAPHICS_STORAGE_H
