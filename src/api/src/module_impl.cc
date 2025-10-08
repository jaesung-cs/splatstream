#include "module_impl.h"

#include "vkgs/core/module.h"

namespace vkgs {

Module::Impl::Impl() { module_ = std::make_shared<core::Module>(); }

Module::Impl::~Impl() = default;

}  // namespace vkgs
