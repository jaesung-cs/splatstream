#include "vkgs/gpu/sampler.h"

#include "volk.h"

#include "vkgs/gpu/object.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API SamplerImpl : public Object {
 public:
  void __init__() {
    VkSamplerCreateInfo create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    create_info.magFilter = VK_FILTER_LINEAR;
    create_info.minFilter = VK_FILTER_LINEAR;
    create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.minLod = 0.0f;
    create_info.maxLod = 100.0f;
    vkCreateSampler(device_, &create_info, NULL, &sampler_);
  }

  void __del__() { vkDestroySampler(device_, sampler_, NULL); }

  operator VkSampler() const noexcept { return sampler_; }

 private:
  VkSampler sampler_ = VK_NULL_HANDLE;
};

Sampler Sampler::Create() { return Make<SamplerImpl>(); }

Sampler::operator VkSampler() const { return *impl_; }

}  // namespace gpu
}  // namespace vkgs
