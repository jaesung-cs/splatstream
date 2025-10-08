#include "vkgs/gaussian_splats.h"

#include "gaussian_splats_impl.h"

namespace vkgs {

GaussianSplats::GaussianSplats() : impl_(std::make_shared<Impl>()) {}

GaussianSplats::~GaussianSplats() = default;

}  // namespace vkgs
