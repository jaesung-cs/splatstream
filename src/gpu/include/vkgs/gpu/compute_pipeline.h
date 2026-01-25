#ifndef VKGS_GPU_COMPUTE_PIPELINE_H
#define VKGS_GPU_COMPUTE_PIPELINE_H

#include <cstdint>
#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class ComputePipelineImpl;
class VKGS_GPU_API ComputePipeline : public Handle<ComputePipeline, ComputePipelineImpl> {
 public:
  static ComputePipeline Create(VkPipelineLayout pipeline_layout, const uint32_t* shader, size_t size);

  template <size_t N>
  static ComputePipeline Create(VkPipelineLayout pipeline_layout, const uint32_t (&shader)[N]) {
    return Create(pipeline_layout, shader, N);
  }

  operator VkPipeline() const noexcept;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_COMPUTE_PIPELINE_H
