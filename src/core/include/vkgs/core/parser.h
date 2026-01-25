#ifndef VKGS_CORE_PARSER_H
#define VKGS_CORE_PARSER_H

#include <memory>
#include <string>
#include <cstdint>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/pipeline_layout.h"
#include "vkgs/gpu/compute_pipeline.h"
#include "vkgs/core/export_api.h"

namespace vkgs {

namespace core {

class GaussianSplats;

class ParserImpl;
class VKGS_CORE_API Parser : public Handle<Parser, ParserImpl> {
 public:
  static Parser Create();

  GaussianSplats CreateGaussianSplats(size_t size, const float* means, const float* quats, const float* scales,
                                      const float* opacities, const uint16_t* colors, int sh_degree,
                                      int opacity_degree) const;
  GaussianSplats LoadFromPly(const std::string& path, int sh_degree = -1) const;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_PARSER_H
