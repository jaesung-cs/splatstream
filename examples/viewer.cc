#include <iostream>

#include "vkgs/core/gaussian_splats.h"
#include "vkgs/core/parser.h"
#include "vkgs/core/renderer.h"
#include "vkgs/viewer/viewer.h"

int main() {
  try {
    auto viewer = vkgs::viewer::Viewer::Create();
    auto parser = vkgs::core::Parser::Create();
    auto renderer = vkgs::core::Renderer::Create();
    auto splats = parser->LoadFromPly("./models/bonsai_30000.ply");
    viewer->SetRenderer(renderer);
    viewer->SetSplats(splats);
    viewer->Run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
