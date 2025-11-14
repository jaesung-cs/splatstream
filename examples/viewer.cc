#include "vkgs/viewer/viewer.h"

int main() {
  vkgs::viewer::Init();

  {
    vkgs::viewer::Viewer viewer;
    viewer.Run();
  }

  vkgs::viewer::Terminate();
  return 0;
}
