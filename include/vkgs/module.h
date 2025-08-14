#ifndef VKGS_MODULE_H
#define VKGS_MODULE_H

#include <memory>

#include "vkgs/export_api.h"

namespace vkgs {

class VKGS_API Module {
 public:
  Module();
  ~Module();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace vkgs

#endif  // VKGS_MODULE_H