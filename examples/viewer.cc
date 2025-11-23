#include <iostream>

#include "vkgs/core/parser.h"
#include "vkgs/core/renderer.h"
#include "vkgs/viewer/viewer.h"

int main() {
  try {
    auto viewer = std::make_shared<vkgs::viewer::Viewer>();
    auto parser = std::make_shared<vkgs::core::Parser>();
    auto renderer = std::make_shared<vkgs::core::Renderer>();
    auto splats = parser->LoadFromPly("./models/bonsai_30000.ply");
    viewer->SetRenderer(renderer);
    viewer->SetSplats(splats);
    viewer->Run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
