#include <iostream>
#include <vector>

#include "vkgs/module.h"
#include "vkgs/buffer.h"

int main() {
  std::cout << "Hello vkgs" << std::endl;

  vkgs::Module module;
  std::cout << "device name: " << module.device_name() << std::endl;
  std::cout << "graphics queue index: " << module.graphics_queue_index() << std::endl;
  std::cout << "compute  queue index: " << module.compute_queue_index() << std::endl;
  std::cout << "transfer queue index: " << module.transfer_queue_index() << std::endl;

  vkgs::Buffer buffer(module, 1024);

  std::vector<float> data(256);
  buffer.ToGpu(data.data(), data.size() * sizeof(float));

  std::cout << "Done" << std::endl;

  return 0;
}
