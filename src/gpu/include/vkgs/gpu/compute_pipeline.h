#ifndef VKGS_GPU_COMPUTE_PIPELINE_H
#define VKGS_GPU_COMPUTE_PIPELINE_H

#include "object.h"

#include <cstdint>
#include <memory>

#include "volk.h"

#include "export_api.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API ComputePipeline : public Object {
 public:
  template <size_t N>
  static std::shared_ptr<ComputePipeline> Create(VkDevice device, VkPipelineLayout pipeline_layout,
                                                 const uint32_t (&shader)[N]) {
    return std::make_shared<ComputePipeline>(device, pipeline_layout, shader, N);
  }

 public:
  ComputePipeline(VkDevice device, VkPipelineLayout pipeline_layout, const uint32_t* shader, size_t size);
  ~ComputePipeline() override;

  operator VkPipeline() const noexcept { return pipeline_; }

 private:
  VkDevice device_;
  VkPipeline pipeline_ = VK_NULL_HANDLE;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_COMPUTE_PIPELINE_H
