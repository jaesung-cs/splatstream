#ifndef VKGS_CORE_RENDERING_TASK_H
#define VKGS_CORE_RENDERING_TASK_H

#include <memory>

#include "vkgs/common/handle.h"
#include "vkgs/gpu/queue_task.h"

#include "vkgs/core/export_api.h"
#include "vkgs/core/draw_result.h"

namespace vkgs {
namespace core {

class RenderingTaskImpl;
class VKGS_CORE_API RenderingTask : public Handle<RenderingTask, RenderingTaskImpl> {
 public:
  static RenderingTask Create();

  void SetTask(gpu::QueueTask task);
  void SetDrawResult(const DrawResult& result);

  const DrawResult& draw_result() const;

  void Wait();
};

}  // namespace core
}  // namespace vkgs

#endif  // VKGS_CORE_RENDERING_TASK_H
