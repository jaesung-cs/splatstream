#include "module_impl.h"

#include "vkgs/core/module.h"

#include "vkgs/buffer.h"

#include "buffer_impl.h"

namespace vkgs {

Module::Impl::Impl() {
  module_ = std::make_shared<core::Module>();
  module_->Init();
}

Module::Impl::~Impl() = default;

}  // namespace vkgs
