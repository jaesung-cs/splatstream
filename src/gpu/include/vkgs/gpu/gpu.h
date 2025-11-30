#ifndef VKGS_GPU_GPU_H
#define VKGS_GPU_GPU_H

#include <memory>

#include "export_api.h"
#include "device.h"

namespace vkgs {
namespace gpu {

void VKGS_GPU_API Init(const DeviceCreateInfo& device_info);

Device VKGS_GPU_API GetDevice();

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_GPU_H
