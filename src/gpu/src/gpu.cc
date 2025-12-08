#include "vkgs/gpu/gpu.h"

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {
namespace {

DeviceCreateInfo global_device_info = {};
std::weak_ptr<DeviceImpl> device_impl;

}  // namespace

void Init(const DeviceCreateInfo& device_info) { global_device_info = device_info; }

Device GetDevice() {
  if (device_impl.expired()) {
    auto new_device = Device::Create(global_device_info);
    device_impl = new_device.impl();
    return new_device;
  }
  return Device::FromPtr(device_impl.lock());
}

}  // namespace gpu
}  // namespace vkgs
