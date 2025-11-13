#ifndef VKGS_CORE_RENDERER_H
#define VKGS_CORE_RENDERER_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "export_api.h"

#include "vkgs/core/draw_options.h"

namespace vkgs {
namespace gpu {

class Device;
class TaskMonitor;
class PipelineLayout;
class ComputePipeline;
class GraphicsPipeline;
class Semaphore;

}  // namespace gpu

namespace core {

class Device;
class GaussianSplats;
class RenderingTask;
class Sorter;
class ComputeStorage;
class GraphicsStorage;
class TransferStorage;

class VKGS_CORE_API Renderer {
 public:
  Renderer();
  ~Renderer();

  const std::string& device_name() const noexcept;
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  std::shared_ptr<GaussianSplats> CreateGaussianSplats(size_t size, const float* means, const float* quats,
                                                       const float* scales, const float* opacities,
                                                       const uint16_t* colors, int sh_degree);
  std::shared_ptr<GaussianSplats> LoadFromPly(const std::string& path, int sh_degree = -1);
  std::shared_ptr<RenderingTask> Draw(std::shared_ptr<GaussianSplats> splats, const DrawOptions& draw_options,
                                      uint8_t* dst);

 private:
  std::shared_ptr<Device> core_device_;

  std::shared_ptr<gpu::Device> device_;
  std::shared_ptr<gpu::TaskMonitor> task_monitor_;
  std::shared_ptr<Sorter> sorter_;

  std::shared_ptr<gpu::PipelineLayout> parse_pipeline_layout_;
  std::shared_ptr<gpu::ComputePipeline> parse_ply_pipeline_;
  std::shared_ptr<gpu::ComputePipeline> parse_data_pipeline_;

  std::shared_ptr<gpu::PipelineLayout> compute_pipeline_layout_;
  std::shared_ptr<gpu::ComputePipeline> rank_pipeline_;
  std::shared_ptr<gpu::ComputePipeline> inverse_index_pipeline_;
  std::shared_ptr<gpu::ComputePipeline> projection_pipeline_;

  std::shared_ptr<gpu::PipelineLayout> graphics_pipeline_layout_;
  std::shared_ptr<gpu::GraphicsPipeline> splat_pipeline_;
  std::shared_ptr<gpu::GraphicsPipeline> splat_background_pipeline_;

  struct RingBuffer {
    std::shared_ptr<ComputeStorage> compute_storage;
    std::shared_ptr<GraphicsStorage> graphics_storage;
    std::shared_ptr<TransferStorage> transfer_storage;
    std::shared_ptr<gpu::Semaphore> compute_semaphore;
    std::shared_ptr<gpu::Semaphore> graphics_semaphore;
    std::shared_ptr<gpu::Semaphore> transfer_semaphore;
  };
  std::array<RingBuffer, 2> ring_buffer_;

  uint64_t frame_index_ = 0;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERER_H
