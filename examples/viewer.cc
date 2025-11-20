#include <iostream>

#include "vkgs/viewer/viewer.h"

int main() {
  try {
    vkgs::viewer::Viewer viewer;
    viewer.Run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
