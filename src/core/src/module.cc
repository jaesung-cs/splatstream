#include "vkgs/core/module.h"

#include <iostream>
#include <vector>

#include "vkgs/core/buffer.h"

#include "command.h"
#include "semaphore.h"

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
    if (graphics && graphics_queue_index_ == VK_QUEUE_FAMILY_IGNORED) graphics_queue_index_ = i;
    if (!graphics && compute ||
        graphics && graphics_queue_index_ != i && compute_queue_index_ == VK_QUEUE_FAMILY_IGNORED)
      compute_queue_index_ = i;
    if (!graphics && !compute && transfer && !special_purpose || graphics && graphics_queue_index_ != i &&
                                                                     compute_queue_index_ != i &&
                                                                     transfer_queue_index_ == VK_QUEUE_FAMILY_IGNORED)
      transfer_queue_index_ = i;
  }

  // Device
  float queue_priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos(3);
  queue_create_infos[0] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_create_infos[0].queueFamilyIndex = graphics_queue_index_;
  queue_create_infos[0].queueCount = 1;
  queue_create_infos[0].pQueuePriorities = &queue_priority;
  queue_create_infos[1] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_create_infos[1].queueFamilyIndex = compute_queue_index_;
  queue_create_infos[1].queueCount = 1;
  queue_create_infos[1].pQueuePriorities = &queue_priority;
  queue_create_infos[2] = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_create_infos[2].queueFamilyIndex = transfer_queue_index_;
  queue_create_infos[2].queueCount = 1;
  queue_create_infos[2].pQueuePriorities = &queue_priority;

  std::vector<const char*> device_extensions = {
      VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
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

  vkGetDeviceQueue(device_, graphics_queue_index_, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, compute_queue_index_, 0, &compute_queue_);
  vkGetDeviceQueue(device_, transfer_queue_index_, 0, &transfer_queue_);

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

  // Command pool
  VkCommandPoolCreateInfo command_pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  command_pool_info.queueFamilyIndex = transfer_queue_index_;
  vkCreateCommandPool(device_, &command_pool_info, NULL, &transfer_command_pool_);
}

Module::~Module() {
  WaitIdle();

  vkDestroyCommandPool(device_, transfer_command_pool_, NULL);
  vmaDestroyAllocator(allocator_);
  vkDestroyDevice(device_, NULL);
  vkDestroyDebugUtilsMessengerEXT(instance_, messenger_, NULL);
  vkDestroyInstance(instance_, NULL);

  volkFinalize();
}

void Module::WaitIdle() { vkDeviceWaitIdle(device_); }

void Module::WriteBuffer(std::shared_ptr<Buffer> buffer, void* ptr) {
  // TODO: create command from command pool
  VkCommandBuffer cb;
  VkCommandBufferAllocateInfo command_buffer_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  command_buffer_info.commandPool = transfer_command_pool_;
  command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_info.commandBufferCount = 1;
  vkAllocateCommandBuffers(device_, &command_buffer_info, &cb);

  auto command = std::make_shared<Command>(shared_from_this(), cb);

  std::memcpy(buffer->stage_buffer_map(), ptr, buffer->size());

  VkBufferCopy2 region = {VK_STRUCTURE_TYPE_BUFFER_COPY_2};
  region.srcOffset = 0;
  region.dstOffset = 0;
  region.size = buffer->size();

  VkCopyBufferInfo2 copy_info = {VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2};
  copy_info.srcBuffer = buffer->stage_buffer();
  copy_info.dstBuffer = buffer->buffer();
  copy_info.regionCount = 1;
  copy_info.pRegions = &region;

  VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cb, &begin_info);
  vkCmdCopyBuffer2(cb, &copy_info);
  vkEndCommandBuffer(cb);

  VkCommandBufferSubmitInfo command_buffer_submit_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  command_buffer_submit_info.commandBuffer = cb;

  // TODO: create semaphore from semaphore pool
  auto semaphore = std::make_shared<Semaphore>(shared_from_this());
  auto value = semaphore->value();

  VkSemaphoreSubmitInfo signal_semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
  signal_semaphore_info.semaphore = semaphore->semaphore();
  signal_semaphore_info.value = value + 1;
  signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

  VkSubmitInfo2 submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &command_buffer_submit_info;
  submit_info.signalSemaphoreInfoCount = 1;
  submit_info.pSignalSemaphoreInfos = &signal_semaphore_info;
  vkQueueSubmit2(transfer_queue_, 1, &submit_info, VK_NULL_HANDLE);

  semaphore->SignalBy(command, value + 1);
}

}  // namespace core
}  // namespace vkgs
