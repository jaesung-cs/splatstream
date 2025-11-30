#ifndef VKGS_CORE_RENDERER_H
#define VKGS_CORE_RENDERER_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "export_api.h"

#include "vkgs/core/draw_options.h"

#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/compute_pipeline.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/timer.h"

namespace vkgs {
namespace gpu {

class Device;

}  // namespace gpu

namespace core {

class GaussianSplats;
class RenderingTask;
class Sorter;
class ComputeStorage;
class ScreenSplats;
class GraphicsStorage;
class TransferStorage;

class VKGS_CORE_API Renderer {
 public:
  Renderer();
  ~Renderer();

  const std::string& device_name() const noexcept { return device_name_; }
  uint32_t graphics_queue_index() const noexcept { return graphics_queue_index_; }
  uint32_t compute_queue_index() const noexcept { return compute_queue_index_; }
  uint32_t transfer_queue_index() const noexcept { return transfer_queue_index_; }

  std::shared_ptr<RenderingTask> Draw(std::shared_ptr<GaussianSplats> splats, const DrawOptions& draw_options,
                                      uint8_t* dst);

  // Low-level API
  /**
   * @brief Compute screen splats in compute queue, and release to graphics queue.
   */
  void ComputeScreenSplats(VkCommandBuffer command_buffer, std::shared_ptr<GaussianSplats> splats,
                           const DrawOptions& draw_options, std::shared_ptr<ScreenSplats> screen_splats,
                           gpu::Timer timer = {});

  /**
   * @brief Record rendering commands for screen splats in graphics queue, inside render pass.
   */
  void RenderScreenSplats(VkCommandBuffer command_buffer, std::shared_ptr<GaussianSplats> splats,
                          const DrawOptions& draw_options, std::shared_ptr<ScreenSplats> screen_splats,
                          std::vector<VkFormat> formats, std::vector<uint32_t> locations,
                          VkFormat depth_format = VK_FORMAT_UNDEFINED, bool render_depth = false);

 private:
  std::string device_name_;
  uint32_t graphics_queue_index_;
  uint32_t compute_queue_index_;
  uint32_t transfer_queue_index_;

  std::shared_ptr<Sorter> sorter_;

  gpu::PipelineLayout compute_pipeline_layout_;
  gpu::ComputePipeline rank_pipeline_;
  gpu::ComputePipeline inverse_index_pipeline_;
  gpu::ComputePipeline projection_pipeline_;

  gpu::PipelineLayout graphics_pipeline_layout_;

  struct RingBuffer {
    std::shared_ptr<ComputeStorage> compute_storage;
    std::shared_ptr<ScreenSplats> screen_splats;
    std::shared_ptr<GraphicsStorage> graphics_storage;
    std::shared_ptr<TransferStorage> transfer_storage;
    gpu::Semaphore compute_semaphore;
    gpu::Semaphore graphics_semaphore;
    gpu::Semaphore transfer_semaphore;
  };
  std::array<RingBuffer, 2> ring_buffer_;

  uint64_t frame_index_ = 0;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERER_H
