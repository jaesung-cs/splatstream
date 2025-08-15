#include "vkgs/module.h"

#include <iostream>
#include <vector>

#include "volk.h"
#include "vk_mem_alloc.h"

namespace vkgs {
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

class Module::Impl {
 public:
  Impl() {
    std::cout << "Impl created" << std::endl;

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
    };

    VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
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
    std::cout << "Physical device: " << device_properties.deviceName << std::endl;

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

      if (graphics) graphics_queue_index_ = i;
      if (!graphics && compute) compute_queue_index_ = i;
      if (!graphics && !compute && transfer && !special_purpose) transfer_queue_index_ = i;
    }

    std::cout << "Graphics queue index: " << graphics_queue_index_ << std::endl;
    std::cout << "Compute  queue index: " << compute_queue_index_ << std::endl;
    std::cout << "Transfer queue index: " << transfer_queue_index_ << std::endl;

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

    VkDeviceCreateInfo device_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = queue_create_infos.size();
    device_info.pQueueCreateInfos = queue_create_infos.data();
    device_info.enabledExtensionCount = 0;
    device_info.ppEnabledExtensionNames = NULL;
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
  }

  ~Impl() {
    std::cout << "Impl destroyed" << std::endl;

    vmaDestroyAllocator(allocator_);
    vkDestroyDevice(device_, NULL);
    vkDestroyDebugUtilsMessengerEXT(instance_, messenger_, NULL);
    vkDestroyInstance(instance_, NULL);

    volkFinalize();
  }

 private:
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  uint32_t graphics_queue_index_ = VK_QUEUE_FAMILY_IGNORED;
  uint32_t compute_queue_index_ = VK_QUEUE_FAMILY_IGNORED;
  uint32_t transfer_queue_index_ = VK_QUEUE_FAMILY_IGNORED;

  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue compute_queue_ = VK_NULL_HANDLE;
  VkQueue transfer_queue_ = VK_NULL_HANDLE;

  VmaAllocator allocator_ = VK_NULL_HANDLE;
};

Module::Module() : impl_(std::make_unique<Impl>()) {}

Module::~Module() = default;

}  // namespace vkgs
