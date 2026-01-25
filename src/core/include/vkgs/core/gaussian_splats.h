#ifndef VKGS_CORE_GAUSSIAN_SPLATS_H
#define VKGS_CORE_GAUSSIAN_SPLATS_H

#include <memory>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/buffer.h"
#include "vkgs/gpu/queue_task.h"
#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class GaussianSplatsImpl;
class VKGS_CORE_API GaussianSplats : public Handle<GaussianSplats, GaussianSplatsImpl> {
 public:
  static GaussianSplats Create(size_t size, uint32_t sh_degree, int opacity_degree, gpu::Buffer position_opacity,
                               gpu::Buffer cov3d, gpu::Buffer sh, gpu::Buffer opacity_sh, gpu::Buffer index_buffer,
                               gpu::QueueTask task);

  size_t size() const;
  uint32_t sh_degree() const;
  int opacity_degree() const;
  gpu::Buffer position_opacity() const;
  gpu::Buffer cov3d() const;
  gpu::Buffer sh() const;
  gpu::Buffer opacity_sh() const;
  gpu::Buffer index_buffer() const;

  void Wait() const;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_GAUSSIAN_SPLATS_H
