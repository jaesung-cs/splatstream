#include "vkgs/gpu/object.h"

#include <stdexcept>

#include "vkgs/gpu/command.h"

namespace vkgs {
namespace gpu {

Object::Object() : device_(GetDevice()) {}

Object::~Object() = default;

void Object::Keep() {
  auto command = device_.CurrentCommand();
  if (!command) throw std::runtime_error("No command is bound");
  command->Keep(shared_from_this());
}

}  // namespace gpu
}  // namespace vkgs
