#ifndef VKGS_BUFFER_IMPL_H
#define VKGS_BUFFER_IMPL_H

#include <memory>

#include "vkgs/core/buffer.h"

#include "vkgs/buffer.h"

namespace vkgs {

class Buffer::Impl {
 public:
  Impl(Module& module, size_t size);

  ~Impl();

  auto buffer() const noexcept { return buffer_; }
  size_t size() const noexcept { return buffer_->size(); }

  void ToGpu(void* ptr, size_t size);

 private:
  std::shared_ptr<core::Buffer> buffer_;
};

}  // namespace vkgs

#endif  // VKGS_BUFFER_IMPL_H
