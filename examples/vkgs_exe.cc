#include <iostream>
#include <vector>

#include "vkgs/module.h"

int main() {
  std::cout << "Hello vkgs" << std::endl;

  vkgs::Module module;
  std::cout << "device name: " << module.device_name() << std::endl;
  std::cout << "graphics queue index: " << module.graphics_queue_index() << std::endl;
  std::cout << "compute  queue index: " << module.compute_queue_index() << std::endl;
  std::cout << "transfer queue index: " << module.transfer_queue_index() << std::endl;

  return 0;
}
