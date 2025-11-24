#include "vkgs/core/gaussian_splats.h"

#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/queue_task.h"

namespace vkgs {
namespace core {

GaussianSplats::GaussianSplats(size_t size, uint32_t sh_degree, std::shared_ptr<gpu::Buffer> position,
                               std::shared_ptr<gpu::Buffer> cov3d, std::shared_ptr<gpu::Buffer> sh,
                               std::shared_ptr<gpu::Buffer> opacity, std::shared_ptr<gpu::Buffer> index_buffer,
                               std::shared_ptr<gpu::QueueTask> task)
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
    task_ = nullptr;
  }
}

}  // namespace core
}  // namespace vkgs
