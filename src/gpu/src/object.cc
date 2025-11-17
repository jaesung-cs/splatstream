#include "vkgs/gpu/object.h"

#include <stdexcept>

#include "vkgs/gpu/device.h"
#include "vkgs/gpu/task.h"

namespace vkgs {
namespace gpu {

Object::Object(std::shared_ptr<Device> device) : device_(device) {}

Object::~Object() = default;

void Object::Keep() {
  auto task = device_->CurrentTask();
  if (!task) throw std::runtime_error("No task is bound");
  task->Keep(shared_from_this());
}

}  // namespace gpu
}  // namespace vkgs
