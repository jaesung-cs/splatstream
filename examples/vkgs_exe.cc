#include <iostream>

#include "vkgs/module.h"
#include "vkgs/gaussian_splats.h"
#include "vkgs/rendered_image.h"

int main() {
  std::cout << "Hello vkgs" << std::endl;

  vkgs::Module module;
  std::cout << "device name: " << module.device_name() << std::endl;
  std::cout << "graphics queue index: " << module.graphics_queue_index() << std::endl;
  std::cout << "compute  queue index: " << module.compute_queue_index() << std::endl;
  std::cout << "transfer queue index: " << module.transfer_queue_index() << std::endl;

  auto gaussian_splats = module.load_from_ply("models/bonsai_30000.ply");
  std::cout << "loaded gaussian splats" << std::endl;
  std::cout << "size: " << gaussian_splats.size() << std::endl;

  return 0;
}
