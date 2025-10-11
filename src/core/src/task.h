#ifndef VKGS_CORE_TASK_H
#define VKGS_CORE_TASK_H

#include <memory>
#include <vector>

namespace vkgs {
namespace core {

class Fence;
class Object;

class Task {
 public:
  Task(std::shared_ptr<Fence> fence, std::vector<std::shared_ptr<Object>> objects);

  ~Task();

  bool IsDone();
  void Wait();

 private:
  std::shared_ptr<Fence> fence_;
  std::vector<std::shared_ptr<Object>> objects_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_TASK_H
