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
  GaussianSplats(size_t size, uint32_t sh_degree, std::shared_ptr<gpu::Buffer> position,
                 std::shared_ptr<gpu::Buffer> cov3d, std::shared_ptr<gpu::Buffer> sh,
                 std::shared_ptr<gpu::Buffer> opacity, std::shared_ptr<gpu::Buffer> index);

  ~GaussianSplats();

  size_t size() const noexcept { return size_; }
  uint32_t sh_degree() const noexcept { return sh_degree_; }
  auto position() const noexcept { return position_; }
  auto cov3d() const noexcept { return cov3d_; }
  auto sh() const noexcept { return sh_; }
  auto opacity() const noexcept { return opacity_; }
  auto index_buffer() const noexcept { return index_buffer_; }

 private:
  size_t size_;
  uint32_t sh_degree_;
  std::shared_ptr<gpu::Buffer> position_;      // (N, 3)
  std::shared_ptr<gpu::Buffer> cov3d_;         // (N, 6)
  std::shared_ptr<gpu::Buffer> sh_;            // (N, K) float16
  std::shared_ptr<gpu::Buffer> opacity_;       // (N)
  std::shared_ptr<gpu::Buffer> index_buffer_;  // (N, 6)
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_GAUSSIAN_SPLATS_H
