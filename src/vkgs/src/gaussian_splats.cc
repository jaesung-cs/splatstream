#include "vkgs/gaussian_splats.h"

#include "vkgs/core/gaussian_splats.h"

namespace vkgs {

class GaussianSplats::Impl {
 public:
  Impl(core::GaussianSplats gaussian_splats) : gaussian_splats_(gaussian_splats) {}

  ~Impl() = default;

  size_t size() const { return gaussian_splats_->size(); }

  void Wait() const { gaussian_splats_->Wait(); }

  core::GaussianSplats get() const noexcept { return gaussian_splats_; }

 private:
  core::GaussianSplats gaussian_splats_;
};

GaussianSplats::GaussianSplats(core::GaussianSplats gaussian_splats) : impl_(std::make_shared<Impl>(gaussian_splats)) {}

GaussianSplats::~GaussianSplats() = default;

size_t GaussianSplats::size() const { return impl_->size(); }

void GaussianSplats::Wait() const { impl_->Wait(); }

core::GaussianSplats GaussianSplats::get() const { return impl_->get(); }

}  // namespace vkgs
