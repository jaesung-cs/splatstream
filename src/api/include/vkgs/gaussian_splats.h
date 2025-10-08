#ifndef VKGS_GAUSSIAN_SPLATS_H
#define VKGS_GAUSSIAN_SPLATS_H

#include <memory>

#include "vkgs/export_api.h"

namespace vkgs {

class VKGS_API GaussianSplats {
 public:
  GaussianSplats();
  ~GaussianSplats();

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_GAUSSIAN_SPLATS_H
