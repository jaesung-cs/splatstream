#ifndef VKGS_GAUSSIAN_SPLATS_H
#define VKGS_GAUSSIAN_SPLATS_H

#include <memory>

#include "vkgs/export_api.h"

namespace vkgs {
namespace core {
class GaussianSplats;
}

class VKGS_API GaussianSplats {
 public:
  explicit GaussianSplats(std::shared_ptr<core::GaussianSplats> gaussian_splats);
  ~GaussianSplats();

  size_t size() const;

  // Internal
  auto get() const noexcept { return gaussian_splats_; }

 private:
  std::shared_ptr<core::GaussianSplats> gaussian_splats_;
};

}  // namespace vkgs

#endif  // VKGS_GAUSSIAN_SPLATS_H
