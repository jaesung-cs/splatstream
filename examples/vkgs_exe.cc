#include <iostream>

#include "vkgs/vkgs.h"
#include "vkgs/engine.h"

int main() {
  std::cout << "Hello vkgs" << std::endl;

  vkgs::Init();

  {
    vkgs::Engine engine;
    std::cout << "device name: " << engine.device_name() << std::endl;
    std::cout << "graphics queue index: " << engine.graphics_queue_index() << std::endl;
    std::cout << "compute  queue index: " << engine.compute_queue_index() << std::endl;
    std::cout << "transfer queue index: " << engine.transfer_queue_index() << std::endl;
  }

  vkgs::Terminate();
  return 0;
}
