#ifndef VKGS_CORE_RENDERING_TASK_H
#define VKGS_CORE_RENDERING_TASK_H

#include <memory>

#include "export_api.h"

namespace vkgs {
namespace gpu {

class Task;

}  // namespace gpu

namespace core {

class VKGS_CORE_API RenderingTask {
 public:
  RenderingTask(std::shared_ptr<gpu::Task> task);
  ~RenderingTask();

  void Wait();

 private:
  std::shared_ptr<gpu::Task> task_;
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERING_TASK_H
