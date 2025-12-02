#ifndef VKGS_GPU_SAMPLER_H
#define VKGS_GPU_SAMPLER_H

#include <memory>

#include <vulkan/vulkan.h>

#include "vkgs/common/shared_accessor.h"
#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API SamplerImpl : public Object {
 public:
  SamplerImpl();
  ~SamplerImpl() override;

  operator VkSampler() const noexcept { return sampler_; }

 private:
  VkSampler sampler_ = VK_NULL_HANDLE;
};

class VKGS_GPU_API Sampler : public SharedAccessor<Sampler, SamplerImpl> {};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SAMPLER_H
