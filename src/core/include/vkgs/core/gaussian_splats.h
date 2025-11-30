#ifndef VKGS_CORE_GAUSSIAN_SPLATS_H
#define VKGS_CORE_GAUSSIAN_SPLATS_H

#include <memory>

#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/queue_task.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class VKGS_CORE_API GaussianSplats {
 public:
  GaussianSplats(size_t size, uint32_t sh_degree, gpu::Buffer position, gpu::Buffer cov3d, gpu::Buffer sh,
                 gpu::Buffer opacity, gpu::Buffer index_buffer, gpu::QueueTask task);

  ~GaussianSplats();

  size_t size() const noexcept { return size_; }
  uint32_t sh_degree() const noexcept { return sh_degree_; }
  auto position() const noexcept { return position_; }
  auto cov3d() const noexcept { return cov3d_; }
  auto sh() const noexcept { return sh_; }
  auto opacity() const noexcept { return opacity_; }
  auto index_buffer() const noexcept { return index_buffer_; }

  void Wait();

 private:
  size_t size_;
  uint32_t sh_degree_;
  gpu::Buffer position_;      // (N, 3)
  gpu::Buffer cov3d_;         // (N, 6)
  gpu::Buffer sh_;            // (N, K) float16
  gpu::Buffer opacity_;       // (N)
  gpu::Buffer index_buffer_;  // (N, 6)
  gpu::QueueTask task_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_GAUSSIAN_SPLATS_H
