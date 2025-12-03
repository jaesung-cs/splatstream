#ifndef VKGS_CORE_RENDERER_H
#define VKGS_CORE_RENDERER_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
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

class VKGS_CORE_API RendererImpl {
 public:
  RendererImpl();
  ~RendererImpl();

  const std::string& device_name() const noexcept { return device_name_; }
  uint32_t graphics_queue_index() const noexcept { return graphics_queue_index_; }
  uint32_t compute_queue_index() const noexcept { return compute_queue_index_; }
  uint32_t transfer_queue_index() const noexcept { return transfer_queue_index_; }

  RenderingTask Draw(GaussianSplats splats, const DrawOptions& draw_options,
                     const ScreenSplatOptions& screen_splat_options, uint8_t* dst);

  // Low-level API
  /**
   * @brief Compute screen splats in compute queue, and release to graphics queue.
   */
  void ComputeScreenSplats(VkCommandBuffer command_buffer, GaussianSplats splats, const DrawOptions& draw_options,
                           ScreenSplats screen_splats, gpu::Timer timer = {});

  /**
   * @brief Record rendering commands for screen splats in graphics queue, inside render pass.
   */
  void RenderScreenSplatsColor(VkCommandBuffer command_buffer, ScreenSplats screen_splats,
                               const ScreenSplatOptions& screen_splat_options,
                               const RenderTargetOptions& render_target_options);
  void RenderScreenSplatsDepth(VkCommandBuffer command_buffer, ScreenSplats screen_splats,
                               const ScreenSplatOptions& screen_splat_options,
                               const RenderTargetOptions& render_target_options);

 private:
  std::string device_name_;
  uint32_t graphics_queue_index_;
  uint32_t compute_queue_index_;
  uint32_t transfer_queue_index_;

  Sorter sorter_;

  gpu::PipelineLayout compute_pipeline_layout_;
  gpu::ComputePipeline rank_pipeline_;
  gpu::ComputePipeline inverse_index_pipeline_;
  gpu::ComputePipeline projection_pipeline_;

  gpu::PipelineLayout graphics_pipeline_layout_;

  struct RingBuffer {
    ComputeStorage compute_storage;
    ScreenSplats screen_splats;
    GraphicsStorage graphics_storage;
    gpu::Semaphore compute_semaphore;
    gpu::Semaphore graphics_semaphore;
    gpu::Semaphore transfer_semaphore;
  };
  std::array<RingBuffer, 2> ring_buffer_;

  uint64_t frame_index_ = 0;
};

class Renderer : public SharedAccessor<Renderer, RendererImpl> {};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERER_H
