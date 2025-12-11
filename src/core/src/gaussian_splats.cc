#include "vkgs/core/gaussian_splats.h"

namespace vkgs {
namespace core {

GaussianSplatsImpl::GaussianSplatsImpl(size_t size, uint32_t sh_degree, gpu::Buffer position_opacity, gpu::Buffer cov3d,
                                       gpu::Buffer sh, gpu::Buffer index_buffer, gpu::QueueTask task)
    : size_(size),
      sh_degree_(sh_degree),
      position_opacity_(position_opacity),
      cov3d_(cov3d),
      sh_(sh),
      index_buffer_(index_buffer),
      task_(task) {}

GaussianSplatsImpl::~GaussianSplatsImpl() = default;

void GaussianSplatsImpl::Wait() {
  if (task_) {
    task_->Wait();
    task_.reset();
  }
}

}  // namespace core
}  // namespace vkgs
