#ifndef VKGS_RENDERING_TASK_H
#define VKGS_RENDERING_TASK_H

#include <memory>
#include <vector>

#include "vkgs/export_api.h"

namespace vkgs {
namespace core {
class RenderingTask;
}

class VKGS_API RenderingTask {
 public:
  explicit RenderingTask(std::shared_ptr<core::RenderingTask> task);
  ~RenderingTask();

  void Wait() const;

 private:
  std::shared_ptr<core::RenderingTask> task_;
};

}  // namespace vkgs

#endif  // VKGS_RENDERING_TASK_H
