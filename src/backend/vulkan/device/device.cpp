#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <quark/vk/device/details/device.hpp>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

using std::vector;

namespace quark::vk {

namespace {

struct DeviceSelection {
  VkPhysicalDevice physical_device{VK_NULL_HANDLE};
  uint32_t graphics_queue_family_index{0};
  uint32_t present_queue_family_index{0};
};

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  vector<VkSurfaceFormatKHR> formats;
  vector<VkPresentModeKHR> present_modes;
};

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

auto has_required_extensions(VkPhysicalDevice physical_device,
                             const vector<const char *> &required_extensions)
    -> bool {
  return std::ranges::all_of(
      required_extensions, [physical_device](const char *extension_name) {
        return has_device_extension(physical_device, extension_name);
      });
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

auto query_swapchain_support(VkPhysicalDevice physical_device,
                             VkSurfaceKHR surface) -> SwapchainSupportDetails {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                            &details.capabilities);

  uint32_t format_count{0};
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                       nullptr);
  if (format_count > 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
                                         &format_count, details.formats.data());
  }

  uint32_t present_mode_count{0};
  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                            &present_mode_count, nullptr);
  if (present_mode_count > 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                              &present_mode_count,
                                              details.present_modes.data());
  }

  return details;
}

auto pick_physical_device(VkInstance instance, VkSurfaceKHR surface,
                          const vector<const char *> &required_extensions)
    -> std::optional<DeviceSelection> {
  uint32_t device_count{0};
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  if (device_count == 0) {
    return std::nullopt;
  }

  vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

  std::optional<DeviceSelection> fallback;

  for (const VkPhysicalDevice &physical_device : devices) {
    const auto graphics_queue_family_index =
        find_graphics_queue_family(physical_device);
    if (!graphics_queue_family_index.has_value()) {
      continue;
    }

    uint32_t queue_family_count{0};
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device,
                                             &queue_family_count, nullptr);

    std::optional<uint32_t> present_queue_family_index;
    for (auto index{0UZ}; index < queue_family_count; ++index) {
      VkBool32 present_supported = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface,
                                           &present_supported);
      if (present_supported == VK_TRUE) {
        present_queue_family_index = index;
        break;
      }
    }

    if (!present_queue_family_index.has_value()) {
      continue;
    }

    if (!has_required_extensions(physical_device, required_extensions)) {
      continue;
    }

    const auto swapchain_support =
        query_swapchain_support(physical_device, surface);
    if (swapchain_support.formats.empty() ||
        swapchain_support.present_modes.empty()) {
      continue;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    const DeviceSelection selection{
        .physical_device = physical_device,
        .graphics_queue_family_index = *graphics_queue_family_index,
        .present_queue_family_index = *present_queue_family_index,
    };

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      return selection;
    }

    if (!fallback.has_value()) {
      fallback = selection;
    }
  }

  return fallback;
}

} // namespace

void Device::create(const Device::CreateInfo &ci) {
  destroy();

  if (ci.instance == VK_NULL_HANDLE) {
    throw std::runtime_error("Cannot create Vulkan device with null instance");
  }
  if (ci.surface == VK_NULL_HANDLE) {
    throw std::runtime_error("Cannot create Vulkan device with null surface");
  }

  const auto selection =
      pick_physical_device(ci.instance, ci.surface, ci.required_extensions);
  if (!selection.has_value()) {
    throw std::runtime_error("No suitable Vulkan physical device found");
  }

  physical_device_ = selection->physical_device;
  graphics_queue_family_index_ = selection->graphics_queue_family_index;
  present_queue_family_index_ = selection->present_queue_family_index;

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

  vector<const char *> enabled_device_extensions = ci.required_extensions;
  constexpr const char *portability_subset_extension =
      "VK_KHR_portability_subset";
  if (has_device_extension(physical_device_, portability_subset_extension) &&
      !std::ranges::contains(enabled_device_extensions,
                             portability_subset_extension)) {
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

  const VkResult create_result =
      vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
  if (create_result != VK_SUCCESS) {
    destroy();
    throw std::runtime_error("vkCreateDevice failed");
  }

  vkGetDeviceQueue(device_, graphics_queue_family_index_, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, present_queue_family_index_, 0, &present_queue_);
}

void Device::destroy() noexcept {
  if (device_ != VK_NULL_HANDLE) {
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }

  physical_device_ = VK_NULL_HANDLE;
  graphics_queue_ = VK_NULL_HANDLE;
  present_queue_ = VK_NULL_HANDLE;
  graphics_queue_family_index_ = 0;
  present_queue_family_index_ = 0;
}

} // namespace quark::vk
