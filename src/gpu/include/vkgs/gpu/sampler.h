#ifndef VKGS_GPU_SAMPLER_H
#define VKGS_GPU_SAMPLER_H

#include <memory>

#include <vulkan/vulkan.h>

#include "export_api.h"
#include "object.h"

namespace vkgs {
namespace gpu {

class VKGS_GPU_API Sampler : public Object {
 public:
  static std::shared_ptr<Sampler> Create();

 public:
  Sampler();
  ~Sampler() override;

  operator VkSampler() const noexcept { return sampler_; }

 private:
  VkSampler sampler_ = VK_NULL_HANDLE;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_SAMPLER_H
