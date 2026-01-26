#ifndef VKGS_RENDERER_H
#define VKGS_RENDERER_H

#include <memory>
#include <string>

#include "vkgs/export_api.h"

#include "vkgs/draw_options.h"
#include "vkgs/camera_params.h"

namespace vkgs {

class GaussianSplats;
class RenderingTask;

class VKGS_API Engine {
 public:
  Engine();
  ~Engine();

  const std::string& device_name() const noexcept;

  GaussianSplats LoadFromPly(const std::string& path, int sh_degree = -1);
  GaussianSplats CreateGaussianSplats(size_t size, const float* means, const float* quats, const float* scales,
                                      const float* opacities, const uint16_t* colors, int sh_degree,
                                      int opacity_degree);
  RenderingTask Draw(GaussianSplats splats, const DrawOptions& draw_options, uint8_t* dst);

  void AddCamera(const CameraParams& camera_params);
  void ClearCameras();
  void Show(GaussianSplats splats);

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERER_H
