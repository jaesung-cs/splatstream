#include <iostream>

#include "vkgs/viewer/viewer.h"

int main() {
  try {
    vkgs::viewer::Viewer viewer;
    viewer.SetModelPath("./models/bonsai_30000.ply");
    viewer.Run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
