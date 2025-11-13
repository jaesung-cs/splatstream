#include "vkgs/core/core.h"

#include "vkgs/gpu/gpu.h"

namespace vkgs {
namespace core {

void Init() { gpu::Init(); }

void Terminate() { gpu::Terminate(); }

}  // namespace core
}  // namespace vkgs
