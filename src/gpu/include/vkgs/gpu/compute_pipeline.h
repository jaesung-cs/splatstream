#ifndef VKGS_GPU_COMPUTE_PIPELINE_H
#define VKGS_GPU_COMPUTE_PIPELINE_H

#include <cstdint>
#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API ComputePipelineImpl : public Object {
 public:
  ComputePipelineImpl(VkPipelineLayout pipeline_layout, const uint32_t* shader, size_t size);

  template <size_t N>
  ComputePipelineImpl(VkPipelineLayout pipeline_layout, const uint32_t (&shader)[N])
      : ComputePipelineImpl(pipeline_layout, shader, N) {}

  ~ComputePipelineImpl() override;

  operator VkPipeline() const noexcept { return pipeline_; }

 private:
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

class VKGS_GPU_API ComputePipeline : public SharedAccessor<ComputePipeline, ComputePipelineImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_COMPUTE_PIPELINE_H
