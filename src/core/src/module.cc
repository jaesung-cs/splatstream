#include "vkgs/core/module.h"

#include <iostream>
#include <vector>
#include <array>

#include "vkgs/core/buffer.h"

#include "command.h"
#include "semaphore_pool.h"
#include "semaphore.h"
#include "fence_pool.h"
#include "fence.h"
#include "task_monitor.h"
#include "task.h"
#include "sorter.h"
#include "queue.h"

namespace vkgs {
namespace core {

namespace {

VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                        VkDebugUtilsMessageTypeFlagsEXT message_type,
                        const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
  const char* level = nullptr;
  switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      level = "VERBOSE";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      level = "WARNING";
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      level = "ERROR  ";
      break;
    default:
      level = "UNKNOWN";
      break;
  }

  const char* type = nullptr;
  switch (message_type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      type = "GENERAL    ";
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      type = "VALIDATION ";
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      type = "PERFORMANCE";
      break;
    default:
      type = "UNKNOWN    ";
      break;
  }

  std::cerr << "Vulkan Validation [" << level << "] [" << type << "] " << callback_data->pMessage << std::endl;
  return VK_FALSE;
}

}  // namespace

Module::Module() {
  volkInitialize();

  // Instance
  VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.pApplicationName = "vkgs";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
  app_info.pEngineName = "vkgs";
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_4;

  VkDebugUtilsMessengerCreateInfoEXT messenger_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  messenger_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  messenger_info.pfnUserCallback = &debug_callback;

  std::vector<const char*> validation_layers = {
      "VK_LAYER_KHRONOS_validation",
  };

  std::vector<const char*> extensions = {
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#ifdef __APPLE__
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
  };

  VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
#ifdef __APPLE__
  instance_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
  instance_info.pNext = &messenger_info;
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledLayerCount = validation_layers.size();
  instance_info.ppEnabledLayerNames = validation_layers.data();
  instance_info.enabledExtensionCount = extensions.size();
  instance_info.ppEnabledExtensionNames = extensions.data();
  vkCreateInstance(&instance_info, NULL, &instance_);

  volkLoadInstance(instance_);

  vkCreateDebugUtilsMessengerEXT(instance_, &messenger_info, NULL, &messenger_);

  // Physical device
  uint32_t physical_device_count = 0;
  vkEnumeratePhysicalDevices(instance_, &physical_device_count, NULL);
  std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
  vkEnumeratePhysicalDevices(instance_, &physical_device_count, physical_devices.data());
  physical_device_ = physical_devices[0];

  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
  device_name_ = device_properties.deviceName;

  // Queue
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, NULL);
  std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_family_properties.data());

  uint32_t graphics_queue_index = VK_QUEUE_FAMILY_IGNORED;
  uint32_t compute_queue_index = VK_QUEUE_FAMILY_IGNORED;
  uint32_t transfer_queue_index = VK_QUEUE_FAMILY_IGNORED;
  for (uint32_t i = 0; i < queue_family_count; i++) {
    auto type = queue_family_properties[i].queueFlags;

    bool graphics = false;
    bool compute = false;
    bool transfer = false;
    bool special_purpose = false;
    if (type & VK_QUEUE_GRAPHICS_BIT) graphics = true;
    if (type & VK_QUEUE_COMPUTE_BIT) compute = true;
    if (type & VK_QUEUE_TRANSFER_BIT) transfer = true;
    if (type & (VK_QUEUE_VIDEO_DECODE_BIT_KHR | VK_QUEUE_VIDEO_ENCODE_BIT_KHR | VK_QUEUE_OPTICAL_FLOW_BIT_NV |
                VK_QUEUE_DATA_GRAPH_BIT_ARM))
      special_purpose = true;

    // TODO: make exact rule for selecting queue.
    if (graphics && graphics_queue_index == VK_QUEUE_FAMILY_IGNORED) graphics_queue_index = i;
    if (!graphics && compute || graphics && graphics_queue_index != i && compute_queue_index == VK_QUEUE_FAMILY_IGNORED)
      compute_queue_index = i;
    if (!graphics && !compute && transfer && !special_purpose || graphics && graphics_queue_index != i &&
                                                                     compute_queue_index != i &&
                                                                     transfer_queue_index == VK_QUEUE_FAMILY_IGNORED)
      transfer_queue_index = i;
  }

  // Device
  float queue_priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos(3);
  queue_create_infos[0] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_create_infos[0].queueFamilyIndex = graphics_queue_index;
  queue_create_infos[0].queueCount = 1;
  queue_create_infos[0].pQueuePriorities = &queue_priority;
  queue_create_infos[1] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_create_infos[1].queueFamilyIndex = compute_queue_index;
  queue_create_infos[1].queueCount = 1;
  queue_create_infos[1].pQueuePriorities = &queue_priority;
  queue_create_infos[2] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_create_infos[2].queueFamilyIndex = transfer_queue_index;
  queue_create_infos[2].queueCount = 1;
  queue_create_infos[2].pQueuePriorities = &queue_priority;

  std::vector<const char*> device_extensions = {
      VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
      VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
#ifdef __APPLE__
      "VK_KHR_portability_subset",
#endif
  };

  VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
  timeline_semaphore_features.timelineSemaphore = VK_TRUE;

  VkPhysicalDeviceSynchronization2Features synchronization_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
  synchronization_features.pNext = &timeline_semaphore_features;
  synchronization_features.synchronization2 = VK_TRUE;

  VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pNext = &synchronization_features;
  device_info.queueCreateInfoCount = queue_create_infos.size();
  device_info.pQueueCreateInfos = queue_create_infos.data();
  device_info.enabledExtensionCount = device_extensions.size();
  device_info.ppEnabledExtensionNames = device_extensions.data();
  vkCreateDevice(physical_device_, &device_info, NULL, &device_);

  VkQueue graphics_queue;
  VkQueue compute_queue;
  VkQueue transfer_queue;
  vkGetDeviceQueue(device_, graphics_queue_index, 0, &graphics_queue);
  vkGetDeviceQueue(device_, compute_queue_index, 0, &compute_queue);
  vkGetDeviceQueue(device_, transfer_queue_index, 0, &transfer_queue);

  semaphore_pool_ = std::make_shared<SemaphorePool>(this);
  fence_pool_ = std::make_shared<FencePool>(this);
  task_monitor_ = std::make_shared<TaskMonitor>();
  sorter_ = std::make_shared<Sorter>(this);

  graphics_queue_ = std::make_shared<Queue>(this, graphics_queue, graphics_queue_index);
  compute_queue_ = std::make_shared<Queue>(this, compute_queue, compute_queue_index);
  transfer_queue_ = std::make_shared<Queue>(this, transfer_queue, transfer_queue_index);

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

Module::~Module() {
  std::cout << "Module successfully destroyed!" << std::endl;

  WaitIdle();

  graphics_queue_ = nullptr;
  compute_queue_ = nullptr;
  transfer_queue_ = nullptr;
  semaphore_pool_ = nullptr;
  fence_pool_ = nullptr;
  task_monitor_ = nullptr;
  sorter_ = nullptr;

  vmaDestroyAllocator(allocator_);
  vkDestroyDevice(device_, NULL);
  vkDestroyDebugUtilsMessengerEXT(instance_, messenger_, NULL);
  vkDestroyInstance(instance_, NULL);

  volkFinalize();
}

void Module::Init() {}

uint32_t Module::graphics_queue_index() const noexcept { return graphics_queue_->family_index(); }
uint32_t Module::compute_queue_index() const noexcept { return compute_queue_->family_index(); }
uint32_t Module::transfer_queue_index() const noexcept { return transfer_queue_->family_index(); }

void Module::WaitIdle() { vkDeviceWaitIdle(device_); }

void Module::SyncBufferWrite(VkCommandBuffer cb, std::vector<VkSemaphoreSubmitInfo>& wait_semaphore_infos,
                             std::shared_ptr<Buffer> buffer, VkDeviceSize size, std::shared_ptr<Queue> queue,
                             VkPipelineStageFlags2 stage, VkAccessFlags2 access) {
  if (buffer->queue()) {
    if (buffer->queue() == queue) {
      // Barrier
      VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
      barrier.srcStageMask = buffer->write_stage_mask() | buffer->read_stage_mask();
      barrier.srcAccessMask = buffer->write_access_mask();
      barrier.dstStageMask = stage;
      barrier.dstAccessMask = access;
      barrier.buffer = buffer->buffer();
      barrier.offset = 0;
      barrier.size = size;
      VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
      dependency_info.bufferMemoryBarrierCount = 1;
      dependency_info.pBufferMemoryBarriers = &barrier;
      vkCmdPipelineBarrier2(cb, &dependency_info);
    } else if (buffer->queue() != queue) {
      // Semaphore
      VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      wait_semaphore_info.semaphore = buffer->queue()->semaphore()->semaphore();
      wait_semaphore_info.value = buffer->timeline();
      wait_semaphore_info.stageMask = stage;
      wait_semaphore_infos.push_back(wait_semaphore_info);
    }
  }
}

void Module::SyncBufferRead(VkCommandBuffer cb, std::vector<VkSemaphoreSubmitInfo>& wait_semaphore_infos,
                            std::shared_ptr<Buffer> buffer, VkDeviceSize size, std::shared_ptr<Queue> queue,
                            VkPipelineStageFlags2 stage, VkAccessFlags2 access) {
  if (buffer->queue()) {
    if (buffer->queue() == queue && buffer->write_stage_mask()) {
      // Barrier
      VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
      barrier.srcStageMask = buffer->write_stage_mask();
      barrier.srcAccessMask = buffer->write_access_mask();
      barrier.dstStageMask = stage;
      barrier.dstAccessMask = access;
      barrier.buffer = buffer->buffer();
      barrier.offset = 0;
      barrier.size = size;
      VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
      dependency_info.bufferMemoryBarrierCount = 1;
      dependency_info.pBufferMemoryBarriers = &barrier;
      vkCmdPipelineBarrier2(cb, &dependency_info);
    } else if (buffer->queue() != queue) {
      auto src_queue_family_index = buffer->queue()->family_index();
      auto dst_queue_family_index = queue->family_index();
      if (src_queue_family_index != dst_queue_family_index) {
        // Release
        auto rel_command = buffer->queue()->AllocateCommandBuffer();
        VkCommandBuffer rel_cb = rel_command->command_buffer();
        auto fence = fence_pool_->Allocate();

        VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(rel_cb, &begin_info);

        VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        barrier.srcStageMask = buffer->write_stage_mask();
        barrier.srcAccessMask = buffer->write_access_mask();
        barrier.srcQueueFamilyIndex = src_queue_family_index;
        barrier.dstQueueFamilyIndex = dst_queue_family_index;
        barrier.buffer = buffer->buffer();
        barrier.offset = 0;
        barrier.size = size;
        VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency_info.bufferMemoryBarrierCount = 1;
        dependency_info.pBufferMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(rel_cb, &dependency_info);

        vkEndCommandBuffer(rel_cb);

        VkCommandBufferSubmitInfo command_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        command_info.commandBuffer = rel_cb;

        VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signal_semaphore_info.semaphore = buffer->queue()->semaphore()->semaphore();
        signal_semaphore_info.value = buffer->queue()->semaphore()->value() + 1;

        VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submit_info.commandBufferInfoCount = 1;
        submit_info.pCommandBufferInfos = &command_info;
        submit_info.signalSemaphoreInfoCount = 1;
        submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;
        vkQueueSubmit2(buffer->queue()->queue(), 1, &submit_info, fence->fence());

        auto task = std::make_shared<Task>(rel_command, fence, std::vector<std::shared_ptr<Buffer>>{buffer});
        task_monitor_->Add(task);

        buffer->queue()->semaphore()->Increment();
        buffer->SetQueueTimeline(buffer->queue(), buffer->queue()->semaphore()->value());
        buffer->SetReadStageMask(0);
        buffer->SetWriteStageMask(0);
        buffer->SetWriteAccessMask(0);
      }

      // Semaphore
      VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      wait_semaphore_info.semaphore = buffer->queue()->semaphore()->semaphore();
      wait_semaphore_info.value = buffer->timeline();
      wait_semaphore_info.stageMask = stage;
      wait_semaphore_infos.push_back(wait_semaphore_info);

      if (buffer->queue()->family_index() != queue->family_index()) {
        // Acquire
        VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        barrier.dstStageMask = stage;
        barrier.dstAccessMask = access;
        barrier.srcQueueFamilyIndex = src_queue_family_index;
        barrier.dstQueueFamilyIndex = dst_queue_family_index;
        barrier.buffer = buffer->buffer();
        barrier.offset = 0;
        barrier.size = size;
        VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency_info.bufferMemoryBarrierCount = 1;
        dependency_info.pBufferMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cb, &dependency_info);
      }
    }
  }
}

void Module::SyncBufferReadWrite(VkCommandBuffer cb, std::vector<VkSemaphoreSubmitInfo>& wait_semaphore_infos,
                                 std::shared_ptr<Buffer> buffer, VkDeviceSize size, std::shared_ptr<Queue> queue,
                                 VkPipelineStageFlags2 read_stage, VkAccessFlags2 read_access,
                                 VkPipelineStageFlags2 write_stage, VkAccessFlags2 write_access) {
  if (buffer->queue()) {
    if (buffer->queue() == queue) {
      // Barrier
      std::array<VkBufferMemoryBarrier2, 2> barriers = {};
      barriers[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
      barriers[0].srcStageMask = buffer->write_stage_mask();
      barriers[0].srcAccessMask = buffer->write_access_mask();
      barriers[0].dstStageMask = read_stage;
      barriers[0].dstAccessMask = read_access;
      barriers[0].buffer = buffer->buffer();
      barriers[0].offset = 0;
      barriers[0].size = size;
      barriers[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
      barriers[1].srcStageMask = buffer->write_stage_mask() | buffer->read_stage_mask();
      barriers[1].srcAccessMask = buffer->write_access_mask();
      barriers[1].dstStageMask = write_stage;
      barriers[1].dstAccessMask = write_access;
      barriers[1].buffer = buffer->buffer();
      barriers[1].offset = 0;
      barriers[1].size = size;
      VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
      dependency_info.bufferMemoryBarrierCount = barriers.size();
      dependency_info.pBufferMemoryBarriers = barriers.data();
      vkCmdPipelineBarrier2(cb, &dependency_info);
    } else if (buffer->queue() != queue) {
      auto src_queue_family_index = buffer->queue()->family_index();
      auto dst_queue_family_index = queue->family_index();
      if (src_queue_family_index != dst_queue_family_index) {
        // Release
        auto rel_command = buffer->queue()->AllocateCommandBuffer();
        VkCommandBuffer rel_cb = rel_command->command_buffer();
        auto fence = fence_pool_->Allocate();

        VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(rel_cb, &begin_info);

        VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        barrier.srcStageMask = buffer->write_stage_mask();
        barrier.srcAccessMask = buffer->write_access_mask();
        barrier.srcQueueFamilyIndex = src_queue_family_index;
        barrier.dstQueueFamilyIndex = dst_queue_family_index;
        barrier.buffer = buffer->buffer();
        barrier.offset = 0;
        barrier.size = size;
        VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency_info.bufferMemoryBarrierCount = 1;
        dependency_info.pBufferMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(rel_cb, &dependency_info);

        vkEndCommandBuffer(rel_cb);

        VkCommandBufferSubmitInfo command_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        command_info.commandBuffer = rel_cb;

        VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signal_semaphore_info.semaphore = buffer->queue()->semaphore()->semaphore();
        signal_semaphore_info.value = buffer->queue()->semaphore()->value() + 1;

        VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submit_info.commandBufferInfoCount = 1;
        submit_info.pCommandBufferInfos = &command_info;
        submit_info.signalSemaphoreInfoCount = 1;
        submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;
        vkQueueSubmit2(buffer->queue()->queue(), 1, &submit_info, fence->fence());

        auto task = std::make_shared<Task>(rel_command, fence, std::vector<std::shared_ptr<Buffer>>{buffer});
        task_monitor_->Add(task);

        buffer->queue()->semaphore()->Increment();
        buffer->SetQueueTimeline(buffer->queue(), buffer->queue()->semaphore()->value());
        buffer->SetReadStageMask(0);
        buffer->SetWriteStageMask(0);
        buffer->SetWriteAccessMask(0);
      }

      // Semaphore
      VkSemaphoreSubmitInfo wait_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
      wait_semaphore_info.semaphore = buffer->queue()->semaphore()->semaphore();
      wait_semaphore_info.value = buffer->timeline();
      wait_semaphore_info.stageMask = read_stage;
      wait_semaphore_infos.push_back(wait_semaphore_info);

      if (buffer->queue()->family_index() != queue->family_index()) {
        // Acquire
        VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
        barrier.dstStageMask = read_stage;
        barrier.dstAccessMask = read_access;
        barrier.srcQueueFamilyIndex = src_queue_family_index;
        barrier.dstQueueFamilyIndex = dst_queue_family_index;
        barrier.buffer = buffer->buffer();
        barrier.offset = 0;
        barrier.size = size;
        VkDependencyInfo dependency_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency_info.bufferMemoryBarrierCount = 1;
        dependency_info.pBufferMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cb, &dependency_info);
      }
    }
  }
}

void Module::CpuToBuffer(std::shared_ptr<Buffer> buffer, const void* ptr, size_t size) {
  auto queue = transfer_queue_;
  auto semaphore = queue->semaphore();

  auto command = queue->AllocateCommandBuffer();
  auto cb = command->command_buffer();
  auto fence = fence_pool_->Allocate();

  VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  VkAccessFlags2 access = VK_ACCESS_2_TRANSFER_WRITE_BIT;

  std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos;

  // TODO: wait for stage buffer to be available.
  std::memcpy(buffer->stage_buffer_map(), ptr, size);

  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cb, &begin_info);

  SyncBufferWrite(cb, wait_semaphore_infos, buffer, size, queue, stage, access);

  // Copy
  VkBufferCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
  region.srcOffset = 0;
  region.dstOffset = 0;
  region.size = size;

  VkCopyBufferInfo2 copy_info = {VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2};
  copy_info.srcBuffer = buffer->stage_buffer();
  copy_info.dstBuffer = buffer->buffer();
  copy_info.regionCount = 1;
  copy_info.pRegions = &region;
  vkCmdCopyBuffer2(cb, &copy_info);

  vkEndCommandBuffer(cb);

  VkCommandBufferSubmitInfo command_buffer_submit_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  command_buffer_submit_info.commandBuffer = cb;

  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = queue->semaphore()->semaphore();
  signal_semaphore_info.value = queue->semaphore()->value() + 1;
  signal_semaphore_info.stageMask = stage;

  VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &command_buffer_submit_info;
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;
  vkQueueSubmit2(transfer_queue_->queue(), 1, &submit_info, fence->fence());

  queue->semaphore()->Increment();
  buffer->SetQueueTimeline(queue, queue->semaphore()->value());
  buffer->SetReadStageMask(0);
  buffer->SetWriteStageMask(stage);
  buffer->SetWriteAccessMask(access);
  task_monitor_->Add(std::make_shared<Task>(command, fence, std::vector<std::shared_ptr<Buffer>>{buffer}));
}

void Module::BufferToCpu(std::shared_ptr<Buffer> buffer, void* ptr, size_t size) {
  auto queue = transfer_queue_;
  auto command = queue->AllocateCommandBuffer();
  auto cb = command->command_buffer();

  auto fence = fence_pool_->Allocate();

  VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  VkAccessFlags2 access = VK_ACCESS_2_TRANSFER_READ_BIT;

  std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos;

  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cb, &begin_info);

  SyncBufferRead(cb, wait_semaphore_infos, buffer, size, queue, stage, access);

  // Copy
  VkBufferCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
  region.srcOffset = 0;
  region.dstOffset = 0;
  region.size = size;
  VkCopyBufferInfo2 copy_info = {VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2};
  copy_info.srcBuffer = buffer->buffer();
  copy_info.dstBuffer = buffer->stage_buffer();
  copy_info.regionCount = 1;
  copy_info.pRegions = &region;
  vkCmdCopyBuffer2(cb, &copy_info);

  vkEndCommandBuffer(cb);

  VkCommandBufferSubmitInfo command_buffer_submit_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  command_buffer_submit_info.commandBuffer = cb;

  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = queue->semaphore()->semaphore();
  signal_semaphore_info.value = queue->semaphore()->value() + 1;
  signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

  VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submit_info.waitSemaphoreInfoCount = wait_semaphore_infos.size();
  submit_info.pWaitSemaphoreInfos = wait_semaphore_infos.data();
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &command_buffer_submit_info;
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;
  vkQueueSubmit2(transfer_queue_->queue(), 1, &submit_info, fence->fence());

  queue->semaphore()->Increment();
  buffer->SetQueueTimeline(queue, queue->semaphore()->value());
  buffer->AddReadStageMask(stage);

  auto task = std::make_shared<Task>(command, fence, std::vector<std::shared_ptr<Buffer>>{buffer});
  task_monitor_->Add(task);

  task->Wait();
  std::memcpy(ptr, buffer->stage_buffer_map(), size);
}

void Module::FillBuffer(std::shared_ptr<Buffer> buffer, uint32_t value) {
  auto queue = transfer_queue_;
  auto command = queue->AllocateCommandBuffer();
  auto cb = command->command_buffer();
  auto fence = fence_pool_->Allocate();

  VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  VkAccessFlags2 access = VK_ACCESS_2_TRANSFER_WRITE_BIT;

  std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos;

  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cb, &begin_info);

  SyncBufferWrite(cb, wait_semaphore_infos, buffer, buffer->size(), queue, stage, access);

  vkCmdFillBuffer(cb, buffer->buffer(), 0, buffer->size(), value);

  vkEndCommandBuffer(cb);

  VkCommandBufferSubmitInfo command_buffer_submit_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  command_buffer_submit_info.commandBuffer = cb;

  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = queue->semaphore()->semaphore();
  signal_semaphore_info.value = queue->semaphore()->value() + 1;
  signal_semaphore_info.stageMask = stage;

  VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submit_info.waitSemaphoreInfoCount = wait_semaphore_infos.size();
  submit_info.pWaitSemaphoreInfos = wait_semaphore_infos.data();
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &command_buffer_submit_info;
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;
  vkQueueSubmit2(transfer_queue_->queue(), 1, &submit_info, fence->fence());

  queue->semaphore()->Increment();
  buffer->SetQueueTimeline(queue, queue->semaphore()->value());
  buffer->SetReadStageMask(0);
  buffer->SetWriteStageMask(stage);
  buffer->SetWriteAccessMask(access);

  task_monitor_->Add(std::make_shared<Task>(command, fence, std::vector<std::shared_ptr<Buffer>>{buffer}));
}

void Module::SortBuffer(std::shared_ptr<Buffer> buffer) {
  auto queue = compute_queue_;
  auto command = queue->AllocateCommandBuffer();
  auto cb = command->command_buffer();
  auto fence = fence_pool_->Allocate();

  VkPipelineStageFlags2 read_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  VkPipelineStageFlags2 write_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  VkAccessFlags2 read_access = VK_ACCESS_2_SHADER_READ_BIT;
  VkAccessFlags2 write_access = VK_ACCESS_2_SHADER_WRITE_BIT;

  std::vector<VkSemaphoreSubmitInfo> wait_semaphore_infos;

  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cb, &begin_info);

  SyncBufferReadWrite(cb, wait_semaphore_infos, buffer, buffer->size(), queue, read_stage, read_access, write_stage,
                      write_access);

  sorter_->Sort(cb, buffer);

  vkEndCommandBuffer(cb);

  VkCommandBufferSubmitInfo command_buffer_submit_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  command_buffer_submit_info.commandBuffer = cb;

  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = queue->semaphore()->semaphore();
  signal_semaphore_info.value = queue->semaphore()->value() + 1;
  signal_semaphore_info.stageMask = write_stage;

  VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submit_info.waitSemaphoreInfoCount = wait_semaphore_infos.size();
  submit_info.pWaitSemaphoreInfos = wait_semaphore_infos.data();
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &command_buffer_submit_info;
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;
  vkQueueSubmit2(compute_queue_->queue(), 1, &submit_info, fence->fence());

  queue->semaphore()->Increment();
  buffer->SetQueueTimeline(queue, queue->semaphore()->value());
  buffer->SetReadStageMask(0);
  buffer->SetWriteStageMask(write_stage);
  buffer->SetWriteAccessMask(write_access);

  task_monitor_->Add(std::make_shared<Task>(command, fence, std::vector<std::shared_ptr<Buffer>>{buffer}));
}

}  // namespace core
}  // namespace vkgs
