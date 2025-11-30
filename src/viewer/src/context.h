#ifndef VKGS_VIEWER_CONTEXT_H
#define VKGS_VIEWER_CONTEXT_H

#include <memory>

#include "vkgs/gpu/device.h"

namespace vkgs {
namespace viewer {

class Context;
std::shared_ptr<Context> GetContext();

class Context {
 public:
  Context();
  ~Context();

  auto device() const noexcept { return device_; }

 private:
  gpu::Device device_;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_CONTEXT_H
