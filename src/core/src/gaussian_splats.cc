#include "vkgs/core/gaussian_splats.h"

namespace vkgs {
namespace core {

class GaussianSplatsImpl {
 public:
  void __init__(size_t size, uint32_t sh_degree, int opacity_degree, gpu::Buffer position_opacity, gpu::Buffer cov3d,
                gpu::Buffer sh, gpu::Buffer opacity_sh, gpu::Buffer index_buffer, gpu::QueueTask task) {
    size_ = size;
    sh_degree_ = sh_degree;
    opacity_degree_ = opacity_degree;
    position_opacity_ = position_opacity;
    cov3d_ = cov3d;
    sh_ = sh;
    opacity_sh_ = opacity_sh;
    index_buffer_ = index_buffer;
    task_ = task;
  }

  size_t size() const noexcept { return size_; }
  uint32_t sh_degree() const noexcept { return sh_degree_; }
  int opacity_degree() const noexcept { return opacity_degree_; }
  auto position_opacity() const noexcept { return position_opacity_; }
  auto cov3d() const noexcept { return cov3d_; }
  auto sh() const noexcept { return sh_; }
  auto opacity_sh() const noexcept { return opacity_sh_; }
  auto index_buffer() const noexcept { return index_buffer_; }

  void Wait() {
    if (task_) {
      task_.Wait();
      task_.reset();
    }
  }

 private:
  size_t size_;
  uint32_t sh_degree_;
  int opacity_degree_;
  gpu::Buffer position_opacity_;  // (N, 4)
  gpu::Buffer cov3d_;             // (N, 6)
  gpu::Buffer sh_;                // (N, K) float16
  gpu::Buffer opacity_sh_;        // (N, K) float16
  gpu::Buffer index_buffer_;      // (N, 6)
  gpu::QueueTask task_;
};

GaussianSplats GaussianSplats::Create(size_t size, uint32_t sh_degree, int opacity_degree, gpu::Buffer position_opacity,
                                      gpu::Buffer cov3d, gpu::Buffer sh, gpu::Buffer opacity_sh,
                                      gpu::Buffer index_buffer, gpu::QueueTask task) {
  return Make<GaussianSplatsImpl>(size, sh_degree, opacity_degree, position_opacity, cov3d, sh, opacity_sh,
                                  index_buffer, task);
}

size_t GaussianSplats::size() const { return impl_->size(); }
uint32_t GaussianSplats::sh_degree() const { return impl_->sh_degree(); }
int GaussianSplats::opacity_degree() const { return impl_->opacity_degree(); }
gpu::Buffer GaussianSplats::position_opacity() const { return impl_->position_opacity(); }
gpu::Buffer GaussianSplats::cov3d() const { return impl_->cov3d(); }
gpu::Buffer GaussianSplats::sh() const { return impl_->sh(); }
gpu::Buffer GaussianSplats::opacity_sh() const { return impl_->opacity_sh(); }
gpu::Buffer GaussianSplats::index_buffer() const { return impl_->index_buffer(); }

void GaussianSplats::Wait() const { impl_->Wait(); }

}  // namespace core
}  // namespace vkgs
