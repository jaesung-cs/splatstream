#ifndef VKGS_CORE_PARSER_H
#define VKGS_CORE_PARSER_H

#include <memory>
#include <string>
#include <cstdint>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/compute_pipeline.h"

#include "vkgs/core/export_api.h"

namespace vkgs {

namespace core {

class GaussianSplats;

class VKGS_CORE_API ParserImpl {
 public:
  ParserImpl();
  ~ParserImpl();

  GaussianSplats CreateGaussianSplats(size_t size, const float* means, const float* quats, const float* scales,
                                      const float* opacities, const uint16_t* colors, int sh_degree,
                                      int opacity_degree);

  GaussianSplats LoadFromPly(const std::string& path, int sh_degree = -1);

 private:
  gpu::PipelineLayout parse_pipeline_layout_;
  gpu::ComputePipeline parse_ply_pipeline_;
  gpu::ComputePipeline parse_data_pipeline_;
};

class Parser : public SharedAccessor<Parser, ParserImpl> {};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_PARSER_H
