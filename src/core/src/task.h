#ifndef VKGS_CORE_TASK_H
#define VKGS_CORE_TASK_H

#include <memory>
#include <vector>

namespace vkgs {
namespace core {

class Command;
class Semaphore;
class Fence;
class Buffer;

class Task {
 public:
  Task(std::shared_ptr<Command> command, std::shared_ptr<Semaphore> semaphore, std::shared_ptr<Fence> fence,
       std::vector<std::shared_ptr<Buffer>> buffers);

  ~Task();

  bool IsDone();
  void Wait();

 private:
  std::shared_ptr<Command> command_;
  std::shared_ptr<Semaphore> semaphore_;
  std::shared_ptr<Fence> fence_;
  std::vector<std::shared_ptr<Buffer>> buffers_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TASK_H
