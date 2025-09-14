#ifndef VKGS_CORE_TASK_H
#define VKGS_CORE_TASK_H

#include <memory>

namespace vkgs {
namespace core {

class Command;
class Fence;

class Task {
 public:
  Task(std::shared_ptr<Command> command, std::shared_ptr<Fence> fence);

  ~Task();

  bool IsDone();
  void Wait();

 private:
  std::shared_ptr<Command> command_;
  std::shared_ptr<Fence> fence_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TASK_H
