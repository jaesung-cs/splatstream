#include <iostream>

#include "vkgs/vkgs.h"
#include "vkgs/viewer/viewer.h"

int main() {
  vkgs::Init();
  {
    vkgs::viewer::Viewer viewer;
    viewer.Run();
  }
  vkgs::Terminate();
  return 0;
}
