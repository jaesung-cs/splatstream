#include "vkgs/gpu/gpu.h"

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {
namespace {

Device::Weak global_device;

}  // namespace

Device GetDevice(const DeviceCreateInfo& create_info) {
  if (auto handle = global_device.lock()) return handle;
  auto handle = Device::Create(create_info);
  global_device = handle;
  return handle;
}

}  // namespace gpu
}  // namespace vkgs
