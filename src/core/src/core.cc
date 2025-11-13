#include "vkgs/core/core.h"

#include <volk.h>

namespace vkgs {
namespace core {

void Init() { volkInitialize(); }

void Terminate() { volkFinalize(); }

}  // namespace core
}  // namespace vkgs
