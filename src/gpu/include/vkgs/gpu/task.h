#ifndef VKGS_GPU_TASK_H
#define VKGS_GPU_TASK_H

#include <memory>
#include <vector>
#include <functional>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Fence;
class Object;

class VKGS_GPU_API Task {
 public:
  Task(std::shared_ptr<Fence> fence, std::vector<std::shared_ptr<Object>> objects, std::function<void()> callback);

  ~Task();

  bool IsDone();
  void Wait();

 private:
  std::shared_ptr<Fence> fence_;
  std::vector<std::shared_ptr<Object>> objects_;
  std::function<void()> callback_;
};

}  // namespace gpu
}  // namespace vkgs

#endif  // VKGS_GPU_TASK_H
