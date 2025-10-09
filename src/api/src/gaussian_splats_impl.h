#ifndef VKGS_GAUSSIAN_SPLATS_IMPL_H
#define VKGS_GAUSSIAN_SPLATS_IMPL_H

#include "vkgs/gaussian_splats.h"

#include "vkgs/core/gaussian_splats.h"

namespace vkgs {

class GaussianSplats::Impl {
 public:
  Impl();
  ~Impl();

  void SetGaussianSplats(std::shared_ptr<core::GaussianSplats> gaussian_splats) { gaussian_splats_ = gaussian_splats; }

  size_t size() const;

 private:
  std::shared_ptr<core::GaussianSplats> gaussian_splats_;
};

}  // namespace vkgs

#endif  // VKGS_GAUSSIAN_SPLATS_IMPL_H
