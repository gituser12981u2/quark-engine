#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <quark/vk/device/details/device.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

using std::vector;

namespace quark::vk::details {

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

[[nodiscard]] util::Result<vector<VkExtensionProperties>>
enumerate_device_extensions(VkPhysicalDevice physical_device) {
  QUARK_ENSURE(physical_device != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "physical_device is null"));

  while (true) {
    uint32_t count = 0;
    QUARK_VK_TRY(vkEnumerateDeviceExtensionProperties(
        physical_device, /*pLayerName=*/nullptr, &count,
        /*pProperties=*/nullptr));

    vector<VkExtensionProperties> props(count);

    uint32_t written = count;
    const VkResult r = vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &written, props.data());

    if (r == VK_SUCCESS) {
      props.resize(written);
      return props;
    }

    if (r != VK_INCOMPLETE) {
      return util::unexpected(
          vk_error(r, "vkEnumerateDeviceExtensionProperties(data)",
                   std::source_location::current()));
    }
  }
}

[[nodiscard]] bool
has_device_extension_props(const vector<VkExtensionProperties> &props,
                           const char *extension_name) noexcept {
  if (extension_name == nullptr || extension_name[0] == '\0') {
    return false;
  }

  for (const auto &p : props) {
    if (std::strcmp(p.extensionName, extension_name) == 0) {
      return true;
    }
  }

  return false;
}

[[nodiscard]] util::Result<bool>
has_required_extensions(VkPhysicalDevice physical_device,
                        const vector<const char *> &required_extensions) {
  vector<VkExtensionProperties> props;
  QUARK_TRY_ASSIGN(props, enumerate_device_extensions(physical_device));

  for (const char *name : required_extensions) {
    QUARK_ENSURE(name != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                            "required extension name is null"));
    QUARK_ENSURE(
        name[0] != '\0',
        QUARK_ERR(util::Errc::InvalidArg, "required extension name is empty"));

    if (!has_device_extension_props(props, name)) {
      return false;
    }
  }

  return true;
}

[[nodiscard]] std::optional<uint32_t>
find_graphics_queue_family(VkPhysicalDevice physical_device) {
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

[[nodiscard]] SwapchainSupportDetails
query_swapchain_support(VkPhysicalDevice physical_device,
                        VkSurfaceKHR surface) {
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

[[nodiscard]] util::Result<DeviceSelection>
pick_physical_device(VkInstance instance, VkSurfaceKHR surface,
                     const vector<const char *> &required_extensions) {

  // TODO: use instance valid instead of manual check
  QUARK_ENSURE(instance != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "instance is null"));
  QUARK_ENSURE(surface != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "surface is null"));

  uint32_t device_count{0};
  QUARK_VK_TRY(vkEnumeratePhysicalDevices(instance, &device_count,
                                          /*pPhysicalDevices=*/nullptr));

  QUARK_ENSURE(device_count > 0, QUARK_ERR(util::Errc::Unsupported,
                                           "No Vulkan physical devices found"));

  vector<VkPhysicalDevice> devices(device_count);
  QUARK_VK_TRY(
      vkEnumeratePhysicalDevices(instance, &device_count, devices.data()));

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

    bool ok_exts = false;
    QUARK_TRY_ASSIGN(
        ok_exts, has_required_extensions(physical_device, required_extensions));

    if (!ok_exts) {
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

  QUARK_ENSURE(fallback.has_value(),
               QUARK_ERR(util::Errc::Unsupported,
                         "No suitable Vulkan physical device found"));

  return *fallback;
}

[[nodiscard]] util::Result<vector<const char *>>
build_device_extensions(VkPhysicalDevice physical_device,
                        const vector<const char *> &required) {
  vector<VkExtensionProperties> props;
  QUARK_TRY_ASSIGN(props, enumerate_device_extensions(physical_device));

  vector<const char *> enabled = required;

#if defined(__APPLE__)
  // MoltenVK usually needs portability subset;
  constexpr const char *kPortabilitySubset = "VK_KHR_portability_subset";
  if (has_device_extension_props(props, kPortabilitySubset) &&
      !std::ranges::contains(enabled, kPortabilitySubset)) {
    enabled.push_back(kPortabilitySubset);
    QUARK_LOG_INFO("portability subset: enabled");
  }
#endif

  return enabled;
}

} // namespace

util::Status Device::create(const Device::CreateInfo &ci) {
  destroy();

  // TODO: use respective .valid()s
  QUARK_ENSURE(ci.instance != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg,
                         "Cannot create Vulkan device with null instance"));
  QUARK_ENSURE(ci.surface != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg,
                         "Cannot create Vulkan device with null surface"));

  DeviceSelection selection{};
  QUARK_TRY_ASSIGN(selection, pick_physical_device(ci.instance, ci.surface,
                                                   ci.required_extensions));

  physical_device_ = selection.physical_device;
  graphics_queue_family_index_ = selection.graphics_queue_family_index;
  present_queue_family_index_ = selection.present_queue_family_index;

  constexpr float queue_priority{1.0F};

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

  vector<const char *> enabled_device_extensions;
  QUARK_TRY_ASSIGN(
      enabled_device_extensions,
      build_device_extensions(physical_device_, ci.required_extensions));

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(enabled_device_extensions.size());
  create_info.ppEnabledExtensionNames = enabled_device_extensions.empty()
                                            ? nullptr
                                            : enabled_device_extensions.data();

  VkDevice out = VK_NULL_HANDLE;
  QUARK_VK_TRY(vkCreateDevice(physical_device_, &create_info,
                              /*pAllocator=*/nullptr, &out));
  device_ = out;

  vkGetDeviceQueue(device_, graphics_queue_family_index_, 0, &graphics_queue_);
  vkGetDeviceQueue(device_, present_queue_family_index_, 0, &present_queue_);

  QUARK_OK();
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

} // namespace quark::vk::details
