#ifndef VKGS_CORE_MODULE_H
#define VKGS_CORE_MODULE_H

#include <cstdint>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "volk.h"
#include "vk_mem_alloc.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class Device;
class TaskMonitor;
class Sorter;
class GaussianSplats;
class PipelineLayout;
class ComputePipeline;
class GraphicsPipeline;
class RenderedImage;

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
  std::shared_ptr<Device> device_;
  std::shared_ptr<TaskMonitor> task_monitor_;
  std::shared_ptr<Sorter> sorter_;

  std::shared_ptr<PipelineLayout> parse_ply_pipeline_layout_;
  std::shared_ptr<ComputePipeline> parse_ply_pipeline_;

  std::shared_ptr<PipelineLayout> rank_pipeline_layout_;
  std::shared_ptr<ComputePipeline> rank_pipeline_;

  std::shared_ptr<PipelineLayout> inverse_index_pipeline_layout_;
  std::shared_ptr<ComputePipeline> inverse_index_pipeline_;

  std::shared_ptr<PipelineLayout> projection_pipeline_layout_;
  std::shared_ptr<ComputePipeline> projection_pipeline_;

  std::shared_ptr<PipelineLayout> splat_pipeline_layout_;
  std::shared_ptr<GraphicsPipeline> splat_pipeline_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_MODULE_H
