#ifndef VKGS_CORE_RENDERER_H
#define VKGS_CORE_RENDERER_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/compute_pipeline.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/timer.h"
#include "vkgs/gpu/buffer.h"

#include "vkgs/core/export_api.h"
#include "vkgs/core/draw_options.h"
#include "vkgs/core/screen_splat_options.h"
#include "vkgs/core/screen_splats.h"
#include "vkgs/core/details/compute_storage.h"
#include "vkgs/core/details/graphics_storage.h"
#include "vkgs/core/details/sorter.h"

namespace vkgs {

namespace core {

class GaussianSplats;
class RenderingTask;

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
  uint32_t graphics_queue_index() const;
  uint32_t compute_queue_index() const;
  uint32_t transfer_queue_index() const;

  RenderingTask Draw(GaussianSplats splats, const DrawOptions& draw_options,
                     const ScreenSplatOptions& screen_splat_options, uint8_t* dst);

  // Low-level API
  /**
   * @brief Compute screen splats in compute queue, and release to graphics queue.
   */
  void ComputeScreenSplats(VkCommandBuffer cb, GaussianSplats splats, const DrawOptions& draw_options,
                           ScreenSplats screen_splats, gpu::Timer timer = {});

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
