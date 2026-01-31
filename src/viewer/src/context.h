#ifndef VKGS_VIEWER_CONTEXT_H
#define VKGS_VIEWER_CONTEXT_H

#include <memory>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/fwd.h"

namespace vkgs {
namespace viewer {

class ContextImpl;
class Context : public Handle<Context, ContextImpl> {
 public:
  static Context Create();

  gpu::Device device() const noexcept;
};

Context GetContext();

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_CONTEXT_H
