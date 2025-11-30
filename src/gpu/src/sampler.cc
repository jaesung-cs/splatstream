#include "vkgs/gpu/sampler.h"

#include <volk.h>

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {

SamplerImpl::SamplerImpl() {
  VkSamplerCreateInfo create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  create_info.magFilter = VK_FILTER_LINEAR;
  create_info.minFilter = VK_FILTER_LINEAR;
  create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  create_info.minLod = 0.0f;
  create_info.maxLod = 100.0f;
  vkCreateSampler(*device_, &create_info, NULL, &sampler_);
}

SamplerImpl::~SamplerImpl() { vkDestroySampler(*device_, sampler_, NULL); }

}  // namespace gpu
}  // namespace vkgs
