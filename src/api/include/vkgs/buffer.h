#ifndef VKGS_BUFFER_H
#define VKGS_BUFFER_H

#include <memory>

#include "vkgs/export_api.h"

namespace vkgs {

class Module;

class VKGS_API Buffer {
 public:
  Buffer(Module& module, size_t size);
  ~Buffer();

  size_t size() const noexcept;

  const auto* impl() const noexcept { return impl_.get(); }

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_BUFFER_H
