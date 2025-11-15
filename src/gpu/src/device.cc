#include "vkgs/gpu/device.h"

#include <iostream>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "vkgs/gpu/queue.h"
#include "vkgs/gpu/semaphore.h"
#include "vkgs/gpu/fence.h"

#include "semaphore_pool.h"
#include "fence_pool.h"
#include "command.h"
#include "task_monitor.h"

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

  std::cerr << "Vulkan Validation [" << level << "] [" << type << "] " << callback_data->pMessage << std::endl
            << std::endl;
  return VK_FALSE;
}

}  // namespace

namespace vkgs {
namespace gpu {

Device::Device(const DeviceCreateInfo& create_info) {
  // Instance
  VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.pApplicationName = "vkgs";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 5);
  app_info.pEngineName = "vkgs";
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 5);
  app_info.apiVersion = VK_API_VERSION_1_4;

  VkDebugUtilsMessengerCreateInfoEXT messenger_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  messenger_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  messenger_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  messenger_info.pfnUserCallback = &debug_callback;

  std::vector<const char*> validation_layers = {
      "VK_LAYER_KHRONOS_validation",
  };

  std::vector<const char*> extensions = create_info.instance_extensions;
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef __APPLE__
  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

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

  // Queue
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, NULL);
  std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_family_properties.data());

  uint32_t compute_queue_index = VK_QUEUE_FAMILY_IGNORED;
  uint32_t graphics_queue_index = VK_QUEUE_FAMILY_IGNORED;
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
#ifdef __APPLE__
      "VK_KHR_portability_subset",
#endif
  };
  if (create_info.enable_viewer) device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  // VkPhysicalDeviceVulkan13Features
  VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
  dynamic_rendering_features.dynamicRendering = VK_TRUE;

  VkPhysicalDeviceSynchronization2Features synchronization_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
  synchronization_features.pNext = &dynamic_rendering_features;
  synchronization_features.synchronization2 = VK_TRUE;

  // VkPhysicalDeviceVulkan12Features
  VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
  timeline_semaphore_features.pNext = &synchronization_features;
  timeline_semaphore_features.timelineSemaphore = VK_TRUE;

  VkPhysicalDeviceHostQueryResetFeatures host_query_reset_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES};
  host_query_reset_features.pNext = &timeline_semaphore_features;
  host_query_reset_features.hostQueryReset = VK_TRUE;

  // VkPhysicalDeviceVulkan11Features
  VkPhysicalDevice16BitStorageFeatures k16bit_storage_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
  k16bit_storage_features.pNext = &host_query_reset_features;
  k16bit_storage_features.storageBuffer16BitAccess = VK_TRUE;

  VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR swapchain_maintenance_features = {
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR};
  swapchain_maintenance_features.pNext = &k16bit_storage_features;
  swapchain_maintenance_features.swapchainMaintenance1 = VK_TRUE;

  VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  device_info.pNext = &swapchain_maintenance_features;
  device_info.queueCreateInfoCount = queue_create_infos.size();
  device_info.pQueueCreateInfos = queue_create_infos.data();
  device_info.enabledExtensionCount = device_extensions.size();
  device_info.ppEnabledExtensionNames = device_extensions.data();
  vkCreateDevice(physical_device_, &device_info, NULL, &device_);
  volkLoadDevice(device_);

  VkQueue graphics_queue;
  VkQueue compute_queue;
  VkQueue transfer_queue;
  vkGetDeviceQueue(device_, graphics_queue_index, 0, &graphics_queue);
  vkGetDeviceQueue(device_, compute_queue_index, 0, &compute_queue);
  vkGetDeviceQueue(device_, transfer_queue_index, 0, &transfer_queue);

  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
  device_name_ = device_properties.deviceName;

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
  VmaAllocator allocator;
  vmaCreateAllocator(&allocator_info, &allocator);
  allocator_ = allocator;

  // Task monitor
  task_monitor_ = std::make_shared<gpu::TaskMonitor>();
}

Device::~Device() {
  WaitIdle();

  graphics_queue_ = nullptr;
  compute_queue_ = nullptr;
  transfer_queue_ = nullptr;
  semaphore_pool_ = nullptr;
  fence_pool_ = nullptr;
  task_monitor_ = nullptr;

  VmaAllocator allocator = static_cast<VmaAllocator>(allocator_);
  vmaDestroyAllocator(allocator);
  vkDestroyDevice(device_, NULL);
  vkDestroyDebugUtilsMessengerEXT(instance_, messenger_, NULL);
  vkDestroyInstance(instance_, NULL);
}

uint32_t Device::graphics_queue_index() const noexcept { return graphics_queue_->family_index(); }
uint32_t Device::compute_queue_index() const noexcept { return compute_queue_->family_index(); }
uint32_t Device::transfer_queue_index() const noexcept { return transfer_queue_->family_index(); }

std::shared_ptr<Semaphore> Device::AllocateSemaphore() { return semaphore_pool_->Allocate(); }
std::shared_ptr<Fence> Device::AllocateFence() { return fence_pool_->Allocate(); }

void Device::WaitIdle() { vkDeviceWaitIdle(device_); }

QueueSubmission Device::ComputeTask(TaskCallback task_callback, std::function<void()> host_callback) {
  return PrepareTask(compute_queue_, task_callback, host_callback);
}
QueueSubmission Device::GraphicsTask(TaskCallback task_callback, std::function<void()> host_callback) {
  return PrepareTask(graphics_queue_, task_callback, host_callback);
}
QueueSubmission Device::TransferTask(TaskCallback task_callback, std::function<void()> host_callback) {
  return PrepareTask(transfer_queue_, task_callback, host_callback);
}

QueueSubmission Device::PrepareTask(std::shared_ptr<Queue> queue, TaskCallback task_callback,
                                    std::function<void()> host_callback) {
  // Keep the lifetime of references in task_callback
  return QueueSubmission(shared_from_this(), queue, std::move(task_callback), std::move(host_callback));
}

std::shared_ptr<Task> Device::AddTask(std::shared_ptr<Fence> fence, std::shared_ptr<Command> command,
                                      std::function<void(VkCommandBuffer)> task_callback,
                                      std::function<void()> host_callback) {
  return task_monitor_->Add(fence, command, std::move(task_callback), std::move(host_callback));
}
}  // namespace gpu
}  // namespace vkgs
