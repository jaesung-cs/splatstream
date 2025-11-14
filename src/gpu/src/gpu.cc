#include "vkgs/gpu/gpu.h"

#include <volk.h>

namespace vkgs {
namespace gpu {

void Init() { volkInitialize(); }

void Terminate() { volkFinalize(); }

}  // namespace gpu
}  // namespace vkgs
