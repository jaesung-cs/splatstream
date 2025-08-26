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
  std::fill(data.begin(), data.end(), 1.0f);
  buffer.ToGpu(data.data(), data.size() * sizeof(float));
  std::fill(data.begin(), data.end(), 2.0f);
  buffer.ToGpu(data.data(), data.size() * sizeof(float));

  std::vector<float> data2(256);
  buffer.ToCpu(data2.data(), data2.size() * sizeof(float));

  for (auto d : data2) std::cout << d << " ";
  std::cout << std::endl;

  std::cout << "Done" << std::endl;

  return 0;
}
