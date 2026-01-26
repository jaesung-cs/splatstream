#ifndef VKGS_CORE_RENDERER_H
#define VKGS_CORE_RENDERER_H

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/core/export_api.h"
#include "vkgs/gpu/fwd.h"

namespace vkgs {
namespace core {

class GaussianSplats;
class RenderingTask;
class DrawOptions;
class ScreenSplatOptions;
class ScreenSplats;

struct RenderTargetOptions {
  std::vector<VkFormat> formats;
  std::vector<uint32_t> locations;
  VkFormat depth_format;
};

class RendererImpl;
class VKGS_CORE_API Renderer : public Handle<Renderer, RendererImpl> {
 public:
  static Renderer Create();

  const std::string& device_name() const;

  RenderingTask Draw(GaussianSplats splats, const DrawOptions& draw_options,
                     const ScreenSplatOptions& screen_splat_options, uint8_t* dst);

  // Low-level API
  /**
   * @brief Compute screen splats in compute queue, and release to graphics queue.
   */
  void ComputeScreenSplats(VkCommandBuffer cb, GaussianSplats splats, const DrawOptions& draw_options,
                           ScreenSplats screen_splats, gpu::Timer timer);

  /**
   * @brief Record rendering commands for screen splats in graphics queue, inside render pass.
   */
  void RenderScreenSplatsColor(VkCommandBuffer cb, ScreenSplats screen_splats,
                               const ScreenSplatOptions& screen_splat_options,
                               const RenderTargetOptions& render_target_options);
  void RenderScreenSplatsDepth(VkCommandBuffer cb, ScreenSplats screen_splats,
                               const ScreenSplatOptions& screen_splat_options,
                               const RenderTargetOptions& render_target_options);
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERER_H
