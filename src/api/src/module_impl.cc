#include "module_impl.h"

#include "vkgs/gaussian_splats.h"

#include "vkgs/core/module.h"

#include "gaussian_splats_impl.h"

namespace vkgs {

Module::Impl::Impl() { module_ = std::make_shared<core::Module>(); }

Module::Impl::~Impl() = default;

GaussianSplats Module::Impl::load_from_ply(const std::string& path) {
  GaussianSplats gaussian_splats;
  gaussian_splats.impl()->SetGaussianSplats(module_->load_from_ply(path));
  return gaussian_splats;
}

}  // namespace vkgs
