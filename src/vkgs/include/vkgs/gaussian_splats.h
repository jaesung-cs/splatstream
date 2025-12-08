#ifndef VKGS_GAUSSIAN_SPLATS_H
#define VKGS_GAUSSIAN_SPLATS_H

#include <memory>

#include "vkgs/export_api.h"

namespace vkgs {
namespace core {
class GaussianSplats;
}

class VKGS_API GaussianSplats {
 public:
  explicit GaussianSplats(core::GaussianSplats gaussian_splats);
  ~GaussianSplats();

  size_t size() const;

  void Wait() const;

  // Internal
  core::GaussianSplats get() const;

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_GAUSSIAN_SPLATS_H
