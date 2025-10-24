#ifndef VKGS_RENDERER_H
#define VKGS_RENDERER_H

#include <memory>
#include <string>

#include "vkgs/export_api.h"

#include "vkgs/draw_options.h"

namespace vkgs {

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

  const auto* impl() const noexcept { return impl_.get(); }

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERER_H
