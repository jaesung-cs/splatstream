#ifndef VKGS_BUFFER_H
#define VKGS_BUFFER_H

#include "vkgs/export_api.h"
#include "vkgs/object.h"
#include "vkgs/module.h"

namespace vkgs {

class VKGS_API Buffer : public Object {
 public:
  Buffer(Module* module, size_t size);
  ~Buffer() override;

  void Destroy() override;

  size_t size() const noexcept;

  const auto* impl() const noexcept { return impl_.get(); }

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_BUFFER_H
