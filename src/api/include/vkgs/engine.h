#ifndef VKGS_RENDERER_H
#define VKGS_RENDERER_H

#include <memory>
#include <string>

#include "vkgs/export_api.h"

#include "vkgs/draw_options.h"

namespace vkgs {
namespace core {

class Parser;
class Renderer;

}  // namespace core

class GaussianSplats;
class RenderingTask;

class VKGS_API Engine {
 public:
  Engine();
  ~Engine();

  const std::string& device_name() const noexcept;
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  GaussianSplats LoadFromPly(const std::string& path, int sh_degree = -1);
  GaussianSplats CreateGaussianSplats(size_t size, const float* means, const float* quats, const float* scales,
                                      const float* opacities, const uint16_t* colors, int sh_degree);
  RenderingTask Draw(GaussianSplats splats, const DrawOptions& draw_options, uint8_t* dst);

 private:
  std::shared_ptr<core::Parser> parser_;
  std::shared_ptr<core::Renderer> renderer_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERER_H
