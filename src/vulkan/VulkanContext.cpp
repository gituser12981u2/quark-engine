#include "VulkanContext.hpp"

// NOLINTBEGIN(misc-include-cleaner)

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

using std::array;
using std::vector;

namespace {

constexpr int kWindowWidth{1280};
constexpr int kWindowHeight{720};
constexpr std::string_view kWindowTitle = "quark-engine";

constexpr bool kEnableValidationLayers =
#ifndef NDEBUG
    true;
#else
    false;
#endif

constexpr array<const char *, 1> kValidationLayers{
    "VK_LAYER_KHRONOS_validation",
};

constexpr const char *kSwapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

struct DeviceSelection {
  VkPhysicalDevice device{VK_NULL_HANDLE};
  uint32_t graphics_queue_family_index{0};
  uint32_t present_queue_family_index{0};
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  vector<VkSurfaceFormatKHR> formats;
  vector<VkPresentModeKHR> present_modes;
};

VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
               VkDebugUtilsMessageTypeFlagsEXT message_type,
               const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
               void *user_data) {
  (void)message_type;
  (void)user_data;

  if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) !=
          0U ||
      (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) !=
          0U) {
    std::cerr << std::format("[Vulkan] {}\n", callback_data->pMessage);
  }
  return VK_FALSE;
}

auto check_validation_layer_support() -> bool {
  uint32_t layer_count{0};
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char *const layer_name : kValidationLayers) {
    const auto found = std::ranges::any_of(
        available_layers, [layer_name](const VkLayerProperties &properties) {
          return std::strcmp(properties.layerName, layer_name) == 0;
        });

    if (!found) {
      return false;
    }
  }

  return true;
}

auto has_instance_extension(const char *extension_name) -> bool {
  uint32_t extension_count{0};
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

  vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                         extensions.data());

  return std::ranges::any_of(
      extensions, [extension_name](const VkExtensionProperties &extension) {
        return std::strcmp(extension.extensionName, extension_name) == 0;
      });
}

auto has_device_extension(VkPhysicalDevice physical_device,
                          const char *extension_name) -> bool {
  uint32_t extension_count{0};
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                       &extension_count, nullptr);

  vector<VkExtensionProperties> extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                       &extension_count, extensions.data());

  return std::ranges::any_of(
      extensions, [extension_name](const VkExtensionProperties &extension) {
        return std::strcmp(extension.extensionName, extension_name) == 0;
      });
}

auto make_debug_messenger_info() -> VkDebugUtilsMessengerCreateInfoEXT {
  VkDebugUtilsMessengerCreateInfoEXT create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = debug_callback;
  return create_info;
}

auto find_graphics_queue_family(VkPhysicalDevice physical_device)
    -> std::optional<uint32_t> {
  uint32_t queue_family_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           nullptr);

  vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           queue_families.data());

  for (auto index{0UZ}; index < queue_family_count; ++index) {
    if ((queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
      return index;
    }
  }

  return std::nullopt;
}

auto query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
    -> SwapchainSupportDetails {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t format_count{0};
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
  if (format_count > 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                         details.formats.data());
  }

  uint32_t present_mode_count{0};
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                            &present_mode_count, nullptr);
  if (present_mode_count > 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &present_mode_count, details.present_modes.data());
  }

  return details;
}

auto pick_physical_device(VkInstance instance, VkSurfaceKHR surface)
    -> std::optional<DeviceSelection> {
  uint32_t device_count{0};
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  if (device_count == 0) {
    return std::nullopt;
  }

  vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  std::optional<DeviceSelection> fallback;

  for (const VkPhysicalDevice &device : devices) {
    const auto graphics_queue_family_index = find_graphics_queue_family(device);
    if (!graphics_queue_family_index.has_value()) {
      continue;
    }

    uint32_t queue_family_count{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             nullptr);

    std::optional<uint32_t> present_queue_family_index;
    for (auto index{0UZ}; index < queue_family_count; ++index) {
      VkBool32 present_supported = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface,
                                           &present_supported);
      if (present_supported == VK_TRUE) {
        present_queue_family_index = index;
        break;
      }
    }

    if (!present_queue_family_index.has_value()) {
      continue;
    }

    if (!has_device_extension(device, kSwapchainExtension)) {
      continue;
    }

    const auto swapchain_support = query_swapchain_support(device, surface);
    if (swapchain_support.formats.empty() ||
        swapchain_support.present_modes.empty()) {
      continue;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device, &properties);

    const DeviceSelection selection{
        .device = device,
        .graphics_queue_family_index = *graphics_queue_family_index,
        .present_queue_family_index = *present_queue_family_index};

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      return selection;
    }

    if (!fallback.has_value()) {
      fallback = selection;
    }
  }

  return fallback;
}

auto choose_swapchain_surface_format(
    const vector<VkSurfaceFormatKHR> &available_formats) -> VkSurfaceFormatKHR {
  for (const VkSurfaceFormatKHR &surface_format : available_formats) {
    if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return surface_format;
    }
  }
  return available_formats.front();
}

auto choose_swapchain_present_mode(
    const vector<VkPresentModeKHR> &available_present_modes)
    -> VkPresentModeKHR {
  for (const VkPresentModeKHR present_mode : available_present_modes) {
    if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return present_mode;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

auto choose_swapchain_extent(GLFWwindow *window,
                             const VkSurfaceCapabilitiesKHR &capabilities)
    -> VkExtent2D {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  int width{0};
  int height{0};
  glfwGetFramebufferSize(window, &width, &height);

  VkExtent2D actual_extent{
      .width = static_cast<uint32_t>(width),
      .height = static_cast<uint32_t>(height),
  };

  actual_extent.width =
      std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                 capabilities.maxImageExtent.width);
  actual_extent.height =
      std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                 capabilities.maxImageExtent.height);
  return actual_extent;
}

auto vk_result_to_string(const VkResult result) -> std::string_view {
  switch (result) {
  case VK_SUCCESS:
    return "VK_SUCCESS";
  case VK_ERROR_LAYER_NOT_PRESENT:
    return "VK_ERROR_LAYER_NOT_PRESENT";
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return "VK_ERROR_EXTENSION_NOT_PRESENT";
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return "VK_ERROR_INCOMPATIBLE_DRIVER";
  case VK_ERROR_OUT_OF_DATE_KHR:
    return "VK_ERROR_OUT_OF_DATE_KHR";
  default:
    return "VK_UNKNOWN_ERROR";
  }
}

void throw_if_vk_failed(VkResult result, std::string_view step) {
  if (result != VK_SUCCESS) {
    throw std::runtime_error(
        std::format("{} failed with {}", step, vk_result_to_string(result)));
  }
}

} // namespace

namespace quark {

VulkanContext::VulkanContext() {
  create_window();
  create_instance();
  create_debug_messenger();
  create_surface();
  pick_device();
  create_logical_device();
  create_command_pool();
  create_swapchain();
  create_swapchain_image_views();
  create_render_pass();
  create_framebuffers();
  create_command_buffers();
  create_sync_objects();
}

VulkanContext::~VulkanContext() {
  if (device_ != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device_);

    for (auto index{0UZ}; index < kMaxFramesInFlight; ++index) {
      if (image_available_semaphores_[index] != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, image_available_semaphores_[index],
                           nullptr);
      }
      if (in_flight_fences_[index] != VK_NULL_HANDLE) {
        vkDestroyFence(device_, in_flight_fences_[index], nullptr);
      }
    }

    cleanup_swapchain();

    if (command_pool_ != VK_NULL_HANDLE) {
      vkDestroyCommandPool(device_, command_pool_, nullptr);
    }

    vkDestroyDevice(device_, nullptr);
  }

  if (debug_messenger_ != VK_NULL_HANDLE) {
    // NOLINT (FFS)
    const auto destroy_fn = std::bit_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
    if (destroy_fn != nullptr) {
      destroy_fn(instance_, debug_messenger_, nullptr);
    }
  }

  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
  }

  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
  }

  if (window_ != nullptr) {
    glfwDestroyWindow(window_);
  }

  if (glfw_initialised_) {
    glfwTerminate();
  }
}

void VulkanContext::run() {
  while (window_ != nullptr && glfwWindowShouldClose(window_) == 0) {
    glfwPollEvents();
    draw_frame();
  }

  vkDeviceWaitIdle(device_);
}

void VulkanContext::create_window() {
  if (glfwInit() == GLFW_FALSE) {
    throw std::runtime_error("glfwInit failed");
  }
  glfw_initialised_ = true;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(kWindowWidth, kWindowHeight,
                             kWindowTitle.data(), // NOLINT
                             nullptr, nullptr);
  if (window_ == nullptr) {
    throw std::runtime_error("glfwCreateWindow failed");
  }
}

void VulkanContext::create_instance() {
  const bool enable_validation_layers =
      kEnableValidationLayers && check_validation_layer_support();

  if (kEnableValidationLayers && !enable_validation_layers) {
    std::cerr << "Validation layers requested, but unavailable. Continuing "
                 "without them.\n";
  }

  uint32_t required_extension_count{0};
  const char **const required_extensions =
      glfwGetRequiredInstanceExtensions(&required_extension_count);
  if (required_extensions == nullptr || required_extension_count == 0) {
    throw std::runtime_error("glfwGetRequiredInstanceExtensions failed");
  }

  vector<const char *> enabled_extensions(
      required_extensions, required_extensions + required_extension_count);

  if (enable_validation_layers &&
      has_instance_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
    enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    enable_debug_messenger_ = true;
  }

  VkInstanceCreateFlags instance_create_flags = 0;
  constexpr const char *portability_enumeration_extension =
      "VK_KHR_portability_enumeration";
  if (has_instance_extension(portability_enumeration_extension)) {
    enabled_extensions.push_back(portability_enumeration_extension);
    instance_create_flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "quark-engine";
  app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
  app_info.pEngineName = "quark";
  app_info.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.flags = instance_create_flags;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(enabled_extensions.size());
  create_info.ppEnabledExtensionNames =
      enabled_extensions.empty() ? nullptr : enabled_extensions.data();

  if (enable_validation_layers) {
    create_info.enabledLayerCount =
        static_cast<uint32_t>(kValidationLayers.size());
    create_info.ppEnabledLayerNames = kValidationLayers.data();

    if (enable_debug_messenger_) {
      debug_create_info = make_debug_messenger_info();
      create_info.pNext = &debug_create_info;
    }
  }

  throw_if_vk_failed(vkCreateInstance(&create_info, nullptr, &instance_),
                     "vkCreateInstance");
}

void VulkanContext::create_debug_messenger() {
  if (!enable_debug_messenger_) {
    return;
  }

  // NOLINT
  const auto create_fn = std::bit_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
  if (create_fn == nullptr) {
    throw std::runtime_error(
        "vkCreateDebugUtilsMessengerEXT function not found");
  }

  const VkDebugUtilsMessengerCreateInfoEXT create_info =
      make_debug_messenger_info();
  throw_if_vk_failed(
      create_fn(instance_, &create_info, nullptr, &debug_messenger_),
      "vkCreateDebugUtilsMessengerEXT");
}

void VulkanContext::create_surface() {
  throw_if_vk_failed(
      glfwCreateWindowSurface(instance_, window_, nullptr, &surface_),
      "glfwCreateWindowSurface");
}

void VulkanContext::pick_device() {
  const auto selection = pick_physical_device(instance_, surface_);
  if (!selection.has_value()) {
    throw std::runtime_error("No suitable Vulkan physical device found");
  }

  physical_device_ = selection->device;
  graphics_queue_family_index_ = selection->graphics_queue_family_index;
  present_queue_family_index_ = selection->present_queue_family_index;

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physical_device_, &properties);
  std::cout << std::format("Selected GPU: {}\n", properties.deviceName);
}

void VulkanContext::create_logical_device() {
  constexpr auto queue_priority{1.0F};

  const std::set<uint32_t> unique_queue_families{graphics_queue_family_index_,
                                                 present_queue_family_index_};

  vector<VkDeviceQueueCreateInfo> queue_create_infos;
  queue_create_infos.reserve(unique_queue_families.size());
  for (const uint32_t queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  vector<const char *> enabled_device_extensions{kSwapchainExtension};
  constexpr const char *portability_subset_extension =
      "VK_KHR_portability_subset";
  if (has_device_extension(physical_device_, portability_subset_extension)) {
    enabled_device_extensions.push_back(portability_subset_extension);
  }

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(enabled_device_extensions.size());
  create_info.ppEnabledExtensionNames = enabled_device_extensions.data();

  throw_if_vk_failed(
      vkCreateDevice(physical_device_, &create_info, nullptr, &device_),
      "vkCreateDevice");

  vkGetDeviceQueue(device_, graphics_queue_family_index_, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, present_queue_family_index_, 0, &present_queue_);
}

void VulkanContext::create_command_pool() {
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = graphics_queue_family_index_;

  throw_if_vk_failed(
      vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_),
      "vkCreateCommandPool");
}

void VulkanContext::create_swapchain() {
  const SwapchainSupportDetails swapchain_support =
      query_swapchain_support(physical_device_, surface_);

  const VkSurfaceFormatKHR surface_format =
      choose_swapchain_surface_format(swapchain_support.formats);
  const VkPresentModeKHR present_mode =
      choose_swapchain_present_mode(swapchain_support.present_modes);
  const VkExtent2D extent =
      choose_swapchain_extent(window_, swapchain_support.capabilities);

  uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
  if (swapchain_support.capabilities.maxImageCount > 0U &&
      image_count > swapchain_support.capabilities.maxImageCount) {
    image_count = swapchain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface_;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  const array<uint32_t, 2> queue_family_indices = {graphics_queue_family_index_,
                                                   present_queue_family_index_};

  if (graphics_queue_family_index_ != present_queue_family_index_) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices.data();
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = swapchain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  throw_if_vk_failed(
      vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_),
      "vkCreateSwapchainKHR");

  vkGetSwapchainImagesKHR(device_, swapchain_, &image_count, nullptr);
  swapchain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(device_, swapchain_, &image_count,
                          swapchain_images_.data());

  swapchain_image_format_ = surface_format.format;
  swapchain_extent_ = extent;
}

void VulkanContext::create_swapchain_image_views() {
  swapchain_image_views_.resize(swapchain_images_.size());

  for (auto index{0U}; index < swapchain_images_.size(); ++index) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swapchain_images_[index];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swapchain_image_format_;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    throw_if_vk_failed(vkCreateImageView(device_, &create_info, nullptr,
                                         &swapchain_image_views_[index]),
                       "vkCreateImageView");
  }
}

void VulkanContext::create_render_pass() {
  VkAttachmentDescription color_attachment{};
  color_attachment.format = swapchain_image_format_;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  throw_if_vk_failed(
      vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_),
      "vkCreateRenderPass");
}

void VulkanContext::create_framebuffers() {
  framebuffers_.resize(swapchain_image_views_.size());

  for (auto index{0U}; index < swapchain_image_views_.size(); ++index) {
    const array<VkImageView, 1> attachments = {swapchain_image_views_[index]};

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = swapchain_extent_.width;
    framebuffer_info.height = swapchain_extent_.height;
    framebuffer_info.layers = 1;

    throw_if_vk_failed(vkCreateFramebuffer(device_, &framebuffer_info, nullptr,
                                           &framebuffers_[index]),
                       "vkCreateFramebuffer");
  }
}

void VulkanContext::create_command_buffers() {
  command_buffers_.resize(framebuffers_.size());

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount =
      static_cast<uint32_t>(command_buffers_.size());

  throw_if_vk_failed(
      vkAllocateCommandBuffers(device_, &alloc_info, command_buffers_.data()),
      "vkAllocateCommandBuffers");

  for (auto index{0U}; index < command_buffers_.size(); ++index) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    throw_if_vk_failed(
        vkBeginCommandBuffer(command_buffers_[index], &begin_info),
        "vkBeginCommandBuffer");

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_;
    render_pass_info.framebuffer = framebuffers_[index];
    render_pass_info.renderArea.offset = {.x = 0, .y = 0};
    render_pass_info.renderArea.extent = swapchain_extent_;

    VkClearValue clear_color{};
    clear_color.color = {{0.08F, 0.08F, 0.1F, 1.0F}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffers_[index], &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(command_buffers_[index]);

    throw_if_vk_failed(vkEndCommandBuffer(command_buffers_[index]),
                       "vkEndCommandBuffer");
  }
}

void VulkanContext::create_sync_objects() {
  images_in_flight_.assign(swapchain_images_.size(), VK_NULL_HANDLE);
  render_finished_semaphores_per_image_.assign(swapchain_images_.size(),
                                               VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t index = 0; index < kMaxFramesInFlight; ++index) {
    throw_if_vk_failed(vkCreateSemaphore(device_, &semaphore_info, nullptr,
                                         &image_available_semaphores_[index]),
                       "vkCreateSemaphore(image available)");
    throw_if_vk_failed(
        vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[index]),
        "vkCreateFence");
  }

  for (VkSemaphore &render_finished_semaphore :
       render_finished_semaphores_per_image_) {
    throw_if_vk_failed(vkCreateSemaphore(device_, &semaphore_info, nullptr,
                                         &render_finished_semaphore),
                       "vkCreateSemaphore(render finished per image)");
  }
}

void VulkanContext::draw_frame() {
  throw_if_vk_failed(vkWaitForFences(device_, 1,
                                     &in_flight_fences_[current_frame_],
                                     VK_TRUE, UINT64_MAX),
                     "vkWaitForFences");

  uint32_t image_index = 0;
  const VkResult acquire_result =
      vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                            image_available_semaphores_[current_frame_],
                            VK_NULL_HANDLE, &image_index);

  if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate_swapchain();
    return;
  }
  throw_if_vk_failed(acquire_result, "vkAcquireNextImageKHR");

  if (images_in_flight_[image_index] != VK_NULL_HANDLE) {
    throw_if_vk_failed(vkWaitForFences(device_, 1,
                                       &images_in_flight_[image_index], VK_TRUE,
                                       UINT64_MAX),
                       "vkWaitForFences(image)");
  }
  images_in_flight_[image_index] = in_flight_fences_[current_frame_];

  throw_if_vk_failed(
      vkResetFences(device_, 1, &in_flight_fences_[current_frame_]),
      "vkResetFences");

  const array<VkSemaphore, 1> wait_semaphores = {
      image_available_semaphores_[current_frame_]};
  const array<VkPipelineStageFlags, 1> wait_stages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  const array<VkSemaphore, 1> signal_semaphores = {
      render_finished_semaphores_per_image_[image_index]};

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores.data();
  submit_info.pWaitDstStageMask = wait_stages.data();
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffers_[image_index];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores.data();

  throw_if_vk_failed(vkQueueSubmit(graphics_queue_, 1, &submit_info,
                                   in_flight_fences_[current_frame_]),
                     "vkQueueSubmit");

  const array<VkSwapchainKHR, 1> swapchains = {swapchain_};
  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores.data();
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains.data();
  present_info.pImageIndices = &image_index;

  const VkResult present_result =
      vkQueuePresentKHR(present_queue_, &present_info);

  if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
      present_result == VK_SUBOPTIMAL_KHR) {
    recreate_swapchain();
  } else {
    throw_if_vk_failed(present_result, "vkQueuePresentKHR");
  }

  current_frame_ = (current_frame_ + 1U) % kMaxFramesInFlight;
}

void VulkanContext::cleanup_swapchain() {
  for (auto *semaphore : render_finished_semaphores_per_image_) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_, semaphore, nullptr);
    }
  }
  render_finished_semaphores_per_image_.clear();

  for (auto *framebuffer : framebuffers_) {
    vkDestroyFramebuffer(device_, framebuffer, nullptr);
  }
  framebuffers_.clear();

  if (!command_buffers_.empty()) {
    vkFreeCommandBuffers(device_, command_pool_,
                         static_cast<uint32_t>(command_buffers_.size()),
                         command_buffers_.data());
    command_buffers_.clear();
  }

  if (render_pass_ != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }

  for (auto *image_view : swapchain_image_views_) {
    vkDestroyImageView(device_, image_view, nullptr);
  }
  swapchain_image_views_.clear();

  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  swapchain_images_.clear();
}

void VulkanContext::recreate_swapchain() {
  int width{0};
  int height{0};
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window_, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device_);

  cleanup_swapchain();
  create_swapchain();
  create_swapchain_image_views();
  create_render_pass();
  create_framebuffers();
  create_command_buffers();

  images_in_flight_.assign(swapchain_images_.size(), VK_NULL_HANDLE);
  render_finished_semaphores_per_image_.assign(swapchain_images_.size(),
                                               VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (auto index{0U}; index < render_finished_semaphores_per_image_.size();
       ++index) {
    throw_if_vk_failed(
        vkCreateSemaphore(device_, &semaphore_info, nullptr,
                          &render_finished_semaphores_per_image_[index]),
        "vkCreateSemaphore(render finished per image recreate)");
  }
}

} // namespace quark

// NOLINTEND(misc-include-cleaner)
