#include "vkgs/gaussian_splats.h"

#include "vkgs/core/gaussian_splats.h"

namespace vkgs {

GaussianSplats::GaussianSplats(std::shared_ptr<core::GaussianSplats> gaussian_splats)
    : gaussian_splats_(gaussian_splats) {}

GaussianSplats::~GaussianSplats() = default;

size_t GaussianSplats::size() const { return gaussian_splats_->size(); }

void GaussianSplats::Wait() const noexcept { gaussian_splats_->Wait(); }

}  // namespace vkgs
