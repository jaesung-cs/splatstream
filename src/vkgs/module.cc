#include "vkgs/module.h"

#include <iostream>

namespace vkgs {

class Module::Impl {
 public:
  Impl() { std::cout << "Impl created" << std::endl; }

  ~Impl() { std::cout << "Impl destroyed" << std::endl; }

 private:
};

Module::Module() : impl_(std::make_unique<Impl>()) {}

Module::~Module() = default;

}  // namespace vkgs
