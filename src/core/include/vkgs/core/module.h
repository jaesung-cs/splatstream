#ifndef VKGS_CORE_MODULE_H
#define VKGS_CORE_MODULE_H

#include <cstdint>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Device;
class TaskMonitor;
class GaussianSplats;
class PipelineLayout;
class ComputePipeline;
class GraphicsPipeline;
class RenderedImage;

}  // namespace gpu

namespace core {

class GaussianSplats;
class RenderedImage;
class Sorter;

class VKGS_CORE_API Module {
 public:
  Module();
  ~Module();

  const std::string& device_name() const noexcept;
  uint32_t graphics_queue_index() const noexcept;
  uint32_t compute_queue_index() const noexcept;
  uint32_t transfer_queue_index() const noexcept;

  std::shared_ptr<GaussianSplats> load_from_ply(const std::string& path);

  std::shared_ptr<RenderedImage> draw(std::shared_ptr<GaussianSplats> splats, const glm::mat4& view,
                                      const glm::mat4& projection, uint32_t width, uint32_t height);

 private:
  std::shared_ptr<gpu::Device> device_;
  std::shared_ptr<gpu::TaskMonitor> task_monitor_;
  std::shared_ptr<Sorter> sorter_;

  std::shared_ptr<gpu::PipelineLayout> parse_ply_pipeline_layout_;
  std::shared_ptr<gpu::ComputePipeline> parse_ply_pipeline_;

  std::shared_ptr<gpu::PipelineLayout> rank_pipeline_layout_;
  std::shared_ptr<gpu::ComputePipeline> rank_pipeline_;

  std::shared_ptr<gpu::PipelineLayout> inverse_index_pipeline_layout_;
  std::shared_ptr<gpu::ComputePipeline> inverse_index_pipeline_;

  std::shared_ptr<gpu::PipelineLayout> projection_pipeline_layout_;
  std::shared_ptr<gpu::ComputePipeline> projection_pipeline_;

  std::shared_ptr<gpu::PipelineLayout> splat_pipeline_layout_;
  std::shared_ptr<gpu::GraphicsPipeline> splat_pipeline_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_MODULE_H
