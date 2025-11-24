#include "vkgs/gpu/gpu.h"

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {
namespace {

DeviceCreateInfo global_device_info = {};
std::weak_ptr<Device> device;

}  // namespace

void Init(const DeviceCreateInfo& device_info) { global_device_info = device_info; }

std::shared_ptr<Device> GetDevice() {
  if (device.expired()) {
    auto new_device = std::make_shared<Device>(global_device_info);
    device = new_device;
    return new_device;
  }
  return device.lock();
}

}  // namespace gpu
}  // namespace vkgs
