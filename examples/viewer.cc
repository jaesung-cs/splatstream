#include <iostream>

#include "vkgs/viewer/viewer.h"

int main() {
  vkgs::viewer::Init();

  try {
    vkgs::viewer::Viewer viewer;
    viewer.Run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  vkgs::viewer::Terminate();
  return 0;
}
