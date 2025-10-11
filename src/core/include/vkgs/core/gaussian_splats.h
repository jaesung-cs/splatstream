#ifndef VKGS_CORE_GAUSSIAN_SPLATS_H
#define VKGS_CORE_GAUSSIAN_SPLATS_H

#include <memory>

#include "volk.h"

#include "vkgs/core/export_api.h"

namespace vkgs {
namespace core {

class Buffer;

class VKGS_CORE_API GaussianSplats {
 public:
  GaussianSplats(size_t size, std::shared_ptr<Buffer> position, std::shared_ptr<Buffer> cov3d,
                 std::shared_ptr<Buffer> sh, std::shared_ptr<Buffer> opacity);
  ~GaussianSplats();

  size_t size() const noexcept { return size_; }

 private:
  size_t size_;
  std::shared_ptr<Buffer> position_;
  std::shared_ptr<Buffer> cov3d_;
  std::shared_ptr<Buffer> sh_;
  std::shared_ptr<Buffer> opacity_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_GAUSSIAN_SPLATS_H
