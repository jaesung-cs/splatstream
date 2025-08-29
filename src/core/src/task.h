#ifndef VKGS_CORE_TASK_H
#define VKGS_CORE_TASK_H

#include <memory>
#include <vector>

namespace vkgs {
namespace core {

class Command;
class Semaphore;
class Fence;

class Task {
 public:
  Task(std::shared_ptr<Command> command, std::vector<std::shared_ptr<Semaphore>> semaphores,
       std::shared_ptr<Fence> fence);

  ~Task();

  bool IsDone();
  void Wait();

 private:
  std::vector<std::shared_ptr<Semaphore>> semaphores_;
  std::shared_ptr<Command> command_;
  std::shared_ptr<Fence> fence_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TASK_H
