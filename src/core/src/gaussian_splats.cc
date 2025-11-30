#include "vkgs/core/gaussian_splats.h"

namespace vkgs {
namespace core {

GaussianSplats::GaussianSplats(size_t size, uint32_t sh_degree, gpu::Buffer position, gpu::Buffer cov3d, gpu::Buffer sh,
                               gpu::Buffer opacity, gpu::Buffer index_buffer, gpu::QueueTask task)
    : size_(size),
      sh_degree_(sh_degree),
      position_(position),
      cov3d_(cov3d),
      sh_(sh),
      opacity_(opacity),
      index_buffer_(index_buffer),
      task_(task) {}

GaussianSplats::~GaussianSplats() = default;

void GaussianSplats::Wait() {
  if (task_) {
    task_->Wait();
    task_.reset();
  }
}

}  // namespace core
}  // namespace vkgs
