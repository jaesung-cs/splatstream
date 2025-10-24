#ifndef VKGS_RENDERER_H
#define VKGS_RENDERER_H

#include <memory>
#include <string>

#include "vkgs/export_api.h"

#include "vkgs/draw_options.h"

namespace vkgs {
namespace core {
class Renderer;
}

class GaussianSplats;
class RenderedImage;

class VKGS_API Renderer {
 public:
  Renderer();
  ~Renderer();

  const std::string& device_name() const noexcept;
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  GaussianSplats LoadFromPly(const std::string& path, int sh_degree = -1);
  RenderedImage Draw(GaussianSplats splats, const DrawOptions& draw_options, uint8_t* dst);

 private:
  std::shared_ptr<core::Renderer> renderer_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERER_H
