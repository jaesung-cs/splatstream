#include "gaussian_splats_impl.h"

namespace vkgs {

GaussianSplats::Impl::Impl() = default;

GaussianSplats::Impl::~Impl() = default;

size_t GaussianSplats::Impl::size() const { return gaussian_splats_ ? gaussian_splats_->size() : 0; }

}  // namespace vkgs
