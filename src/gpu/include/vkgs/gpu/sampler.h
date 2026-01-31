#ifndef VKGS_GPU_SAMPLER_H
#define VKGS_GPU_SAMPLER_H

#include <vulkan/vulkan.h>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/export_api.h"

namespace vkgs {
namespace gpu {

class SamplerImpl;
class VKGS_GPU_API Sampler : public Handle<Sampler, SamplerImpl> {
 public:
  static Sampler Create();

  operator VkSampler() const;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SAMPLER_H
