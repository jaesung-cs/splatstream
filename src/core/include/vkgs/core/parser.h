#ifndef VKGS_CORE_PARSER_H
#define VKGS_CORE_PARSER_H

#include <memory>
#include <string>
#include <cstdint>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class PipelineLayout;
class ComputePipeline;

}  // namespace gpu

namespace core {

class GaussianSplats;

class VKGS_CORE_API Parser {
 public:
  Parser();
  ~Parser();

  std::shared_ptr<GaussianSplats> CreateGaussianSplats(size_t size, const float* means, const float* quats,
                                                       const float* scales, const float* opacities,
                                                       const uint16_t* colors, int sh_degree);

  std::shared_ptr<GaussianSplats> LoadFromPly(const std::string& path, int sh_degree = -1);

 private:
  std::shared_ptr<gpu::PipelineLayout> parse_pipeline_layout_;
  std::shared_ptr<gpu::ComputePipeline> parse_ply_pipeline_;
  std::shared_ptr<gpu::ComputePipeline> parse_data_pipeline_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_PARSER_H
