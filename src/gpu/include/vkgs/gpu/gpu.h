#ifndef VKGS_GPU_GPU_H
#define VKGS_GPU_GPU_H

#include <memory>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Device;
struct DeviceCreateInfo;

void VKGS_GPU_API Init(const DeviceCreateInfo& device_info);

std::shared_ptr<Device> VKGS_GPU_API GetDevice();

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_GPU_H
