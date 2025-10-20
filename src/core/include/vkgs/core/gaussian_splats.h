#ifndef VKGS_CORE_GAUSSIAN_SPLATS_H
#define VKGS_CORE_GAUSSIAN_SPLATS_H

#include <memory>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Buffer;

}

namespace core {

class VKGS_CORE_API GaussianSplats {
 public:
  GaussianSplats(size_t size, std::shared_ptr<gpu::Buffer> position, std::shared_ptr<gpu::Buffer> cov3d,
                 std::shared_ptr<gpu::Buffer> sh, std::shared_ptr<gpu::Buffer> opacity);
  ~GaussianSplats();

  size_t size() const noexcept { return size_; }
  auto position() const noexcept { return position_; }
  auto cov3d() const noexcept { return cov3d_; }
  auto sh() const noexcept { return sh_; }
  auto opacity() const noexcept { return opacity_; }

 private:
  size_t size_;
  std::shared_ptr<gpu::Buffer> position_;
  std::shared_ptr<gpu::Buffer> cov3d_;
  std::shared_ptr<gpu::Buffer> sh_;
  std::shared_ptr<gpu::Buffer> opacity_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_GAUSSIAN_SPLATS_H
