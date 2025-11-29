#ifndef VKGS_VIEWER_CONTEXT_H
#define VKGS_VIEWER_CONTEXT_H

#include <memory>

namespace vkgs {
namespace gpu {
class Device;
}  // namespace gpu
namespace viewer {

class Context;
std::shared_ptr<Context> GetContext();

class Context {
 public:
  Context();
  ~Context();

  auto device() const noexcept { return device_; }

 private:
  std::shared_ptr<gpu::Device> device_;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_CONTEXT_H
