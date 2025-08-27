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

  std::vector<uint32_t> data(256);
  for (int i = 0; i < data.size(); ++i) data[i] = i;
  std::cout << "ToGpu" << std::endl;
  buffer.ToGpu(data.data(), data.size() * sizeof(uint32_t));

  for (int i = 0; i < data.size(); ++i) data[i] = (256 - i) * 1000;
  std::cout << "ToGpu" << std::endl;
  buffer.ToGpu(data.data(), data.size() * sizeof(uint32_t));

  std::vector<uint32_t> data2(256);
  std::cout << "ToCpu" << std::endl;
  buffer.ToCpu(data2.data(), data2.size() * sizeof(uint32_t));
  for (auto d : data2) std::cout << d << " ";
  std::cout << std::endl;

  std::cout << "Sort" << std::endl;
  buffer.Sort();
  std::cout << "ToCpu" << std::endl;
  buffer.ToCpu(data2.data(), data2.size() * sizeof(uint32_t));
  for (auto d : data2) std::cout << d << " ";
  std::cout << std::endl;

  std::cout << "Done" << std::endl;

  return 0;
}
