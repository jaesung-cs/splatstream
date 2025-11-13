#include "vkgs/gpu/device.h"

#include <iostream>
#include <vector>

#include <GLFW/glfw3.h>

#include "vkgs/gpu/queue.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/fence.h"

#include "semaphore_pool.h"
#include "fence_pool.h"

namespace vkgs {
namespace gpu {

Device::Device(const DeviceCreateInfo& create_info) {
  instance_ = create_info.instance;
  physical_device_ = create_info.physical_device;
  device_ = create_info.device;

  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
  device_name_ = device_properties.deviceName;

  auto graphics_queue_index = create_info.graphics_queue_index;
  auto compute_queue_index = create_info.compute_queue_index;
  auto transfer_queue_index = create_info.transfer_queue_index;
  auto graphics_queue = create_info.graphics_queue;
  auto compute_queue = create_info.compute_queue;
  auto transfer_queue = create_info.transfer_queue;

  semaphore_pool_ = std::make_shared<SemaphorePool>(device_);
  fence_pool_ = std::make_shared<FencePool>(device_);

  graphics_queue_ = std::make_shared<Queue>(device_, graphics_queue, graphics_queue_index);
  compute_queue_ = std::make_shared<Queue>(device_, compute_queue, compute_queue_index);
  transfer_queue_ = std::make_shared<Queue>(device_, transfer_queue, transfer_queue_index);

  // Allocator
  VmaVulkanFunctions functions = {};
  functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
  functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocator_info = {};
  allocator_info.physicalDevice = physical_device_;
  allocator_info.device = device_;
  allocator_info.instance = instance_;
  allocator_info.pVulkanFunctions = &functions;
  allocator_info.vulkanApiVersion = VK_API_VERSION_1_4;
  vmaCreateAllocator(&allocator_info, &allocator_);
}

Device::~Device() {
  WaitIdle();
  vmaDestroyAllocator(allocator_);
}

uint32_t Device::graphics_queue_index() const noexcept { return graphics_queue_->family_index(); }
uint32_t Device::compute_queue_index() const noexcept { return compute_queue_->family_index(); }
uint32_t Device::transfer_queue_index() const noexcept { return transfer_queue_->family_index(); }

std::shared_ptr<Semaphore> Device::AllocateSemaphore() { return semaphore_pool_->Allocate(); }
std::shared_ptr<Fence> Device::AllocateFence() { return fence_pool_->Allocate(); }

void Device::WaitIdle() { vkDeviceWaitIdle(device_); }

}  // namespace gpu
}  // namespace vkgs
