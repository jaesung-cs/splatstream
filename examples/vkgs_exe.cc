#include <iostream>

#include "vkgs/vkgs.h"
#include "vkgs/renderer.h"

int main() {
  std::cout << "Hello vkgs" << std::endl;

  vkgs::Init();

  {
    vkgs::Renderer renderer;
    std::cout << "device name: " << renderer.device_name() << std::endl;
    std::cout << "graphics queue index: " << renderer.graphics_queue_index() << std::endl;
    std::cout << "compute  queue index: " << renderer.compute_queue_index() << std::endl;
    std::cout << "transfer queue index: " << renderer.transfer_queue_index() << std::endl;
  }

  vkgs::Terminate();
  return 0;
}
