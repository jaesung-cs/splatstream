#ifndef VKGS_GPU_GPU_H
#define VKGS_GPU_GPU_H

#include "vkgs/gpu/export_api.h"
#include "vkgs/gpu/device.h"

namespace vkgs {
namespace gpu {

Device VKGS_GPU_API GetDevice(const DeviceCreateInfo& create_info = {});

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_GPU_H
