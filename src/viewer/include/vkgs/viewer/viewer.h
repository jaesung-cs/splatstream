#ifndef VKGS_VIEWER_VIEWER_H
#define VKGS_VIEWER_VIEWER_H

#include <memory>

#include "export_api.h"

struct GLFWwindow;

namespace vkgs {
namespace gpu {

class PipelineLayout;
class GraphicsPipeline;

}  // namespace gpu

namespace viewer {

void VKGS_VIEWER_API Init();
void VKGS_VIEWER_API Terminate();

class VKGS_VIEWER_API Viewer {
 public:
  Viewer();
  ~Viewer();

  void Run();

 private:
  GLFWwindow* window_ = nullptr;

  std::shared_ptr<gpu::PipelineLayout> graphics_pipeline_layout_;
  std::shared_ptr<gpu::GraphicsPipeline> blend_pipeline_;
};

}  // namespace viewer
}  // namespace vkgs

#endif  // VKGS_VIEWER_VIEWER_H
