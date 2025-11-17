#include "vkgs/gpu/gpu.h"

#include <volk.h>

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/graphics_pipeline.h"

namespace vkgs {
namespace gpu {
namespace {

std::shared_ptr<Device> device;

}

void Init() { Init({}); }

void Init(const DeviceCreateInfo& device_info) {
  volkInitialize();
  device = std::make_shared<Device>(device_info);
}

void Terminate() {
  GraphicsPipeline::Terminate();
  device = nullptr;
  volkFinalize();
}

std::shared_ptr<Device> GetDevice() { return device; }

}  // namespace gpu
}  // namespace vkgs
