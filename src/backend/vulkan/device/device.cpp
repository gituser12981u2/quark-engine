#include <algorithm>
#include <cstdint>
#include <cstring>
#include <quark/vk/device/details/device.hpp>
#include <quark/vk/device/details/device_feature_chain_builder.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

util::Status Device::create(const Device::CreateInfo &ci) {
  destroy();

  QUARK_ENSURE(ci.instance != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg,
                         "Cannot create Vulkan device with null instance"));
  QUARK_ENSURE(ci.surface != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg,
                         "Cannot create Vulkan device with null surface"));

  alloc_ = ci.allocator;

  DeviceSelection selection{};
  QUARK_TRY_ASSIGN(selection, pick_physical_device_(ci.instance, ci.surface,
                                                    ci.required_extensions));

  physical_device_ = selection.physical_device;
  graphics_queue_family_index_ = selection.graphics_queue_family_index;
  present_queue_family_index_ = selection.present_queue_family_index;

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(physical_device_, &properties);
  capabilities_.api_version = properties.apiVersion;
  capabilities_.supported_features = 0;
  capabilities_.enabled_features = 0;

  QUARK_LOG_INFO("physical device vulkan version: {}.{}.{}",
                 VK_VERSION_MAJOR(capabilities_.api_version),
                 VK_VERSION_MINOR(capabilities_.api_version),
                 VK_VERSION_PATCH(capabilities_.api_version));

  constexpr float queue_priority{1.0F};
  const std::set<uint32_t> unique_queue_families{graphics_queue_family_index_,
                                                 present_queue_family_index_};

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  queue_create_infos.reserve(unique_queue_families.size());
  for (const uint32_t queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  std::vector<const char *> enabled_device_extensions;
  QUARK_TRY_ASSIGN(
      enabled_device_extensions,
      build_device_extensions_(physical_device_, ci.required_extensions));

  DeviceFeatureChainBuilder feature_builder;
  QUARK_TRY_STATUS(
      feature_builder.query_supported(physical_device_, capabilities_));
  QUARK_TRY_STATUS(feature_builder.select_requested(
      ci.required_features, ci.preferred_features, capabilities_));

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = feature_builder.create_pnext();
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(enabled_device_extensions.size());
  create_info.ppEnabledExtensionNames = enabled_device_extensions.empty()
                                            ? nullptr
                                            : enabled_device_extensions.data();
  create_info.pEnabledFeatures = nullptr;

  VkDevice out = VK_NULL_HANDLE;
  QUARK_VK_TRY(vkCreateDevice(physical_device_, &create_info, alloc_, &out));
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
  alloc_ = nullptr;
}

util::Result<Device::DeviceSelection> Device::pick_physical_device_(
    VkInstance instance, VkSurfaceKHR surface,
    const std::vector<const char *> &required_extensions) {
  QUARK_ENSURE(instance != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "instance is null"));
  QUARK_ENSURE(surface != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "surface is null"));

  uint32_t device_count{0};
  QUARK_VK_TRY(vkEnumeratePhysicalDevices(instance, &device_count,
                                          /*pPhysicalDevices=*/nullptr));

  QUARK_ENSURE(device_count > 0, QUARK_ERR(util::Errc::Unsupported,
                                           "No Vulkan physical devices found"));

  std::vector<VkPhysicalDevice> devices(device_count);
  QUARK_VK_TRY(
      vkEnumeratePhysicalDevices(instance, &device_count, devices.data()));

  std::optional<DeviceSelection> fallback;
  auto enumerate = [&](VkPhysicalDevice physical_device)
      -> util::Result<std::vector<VkExtensionProperties>> {
    return enumerate_device_extensions_(physical_device);
  };

  for (const VkPhysicalDevice &physical_device : devices) {
    const auto graphics_queue_family_index =
        find_graphics_queue_family_or_error_(physical_device);
    if (!graphics_queue_family_index.has_value()) {
      continue;
    }

    const auto present_queue_family_index =
        find_present_queue_family_or_error_(physical_device, surface);
    if (!present_queue_family_index.has_value()) {
      continue;
    }

    std::vector<VkExtensionProperties> props;
    QUARK_TRY_ASSIGN(props, enumerate(physical_device));

    bool ok_exts = true;
    for (const char *name : required_extensions) {
      if (!has_device_extension_props_(props, name)) {
        ok_exts = false;
        break;
      }
    }
    if (!ok_exts) {
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

util::Result<std::vector<VkExtensionProperties>>
Device::enumerate_device_extensions_(VkPhysicalDevice physical_device) {
  QUARK_ENSURE(physical_device != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "physical_device is null"));

  while (true) {
    uint32_t count = 0;
    QUARK_VK_TRY(vkEnumerateDeviceExtensionProperties(
        physical_device, /*pLayerName=*/nullptr, &count,
        /*pProperties=*/nullptr));

    std::vector<VkExtensionProperties> props(count);

    uint32_t written = count;
    const VkResult r = vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &written, props.data());

    if (r == VK_SUCCESS) {
      props.resize(written);
      return props;
    }

    if (r != VK_INCOMPLETE) {
      QUARK_FAIL(vk_error(r, "vkEnumerateDeviceExtensionProperties(data)"));
    }
  }
}

bool Device::has_device_extension_props_(
    const std::vector<VkExtensionProperties> &props,
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

util::Result<std::vector<const char *>>
Device::build_device_extensions_(VkPhysicalDevice physical_device,
                                 const std::vector<const char *> &required) {
  std::vector<VkExtensionProperties> props;
  QUARK_TRY_ASSIGN(props, enumerate_device_extensions_(physical_device));

  std::vector<const char *> enabled;
  enabled.reserve(required.size() + 2);

  auto request_one = [&](const char *name) -> util::Status {
    QUARK_ENSURE(name != nullptr,
                 QUARK_ERR(util::Errc::InvalidArg, "extension name is null"));
    QUARK_ENSURE(name[0] != '\0',
                 QUARK_ERR(util::Errc::InvalidArg, "extension name is empty"));

    const bool supported = has_device_extension_props_(props, name);
    QUARK_LOG_INFO("device extension requested: '{}' supported={}", name,
                   supported ? "true" : "false");

    QUARK_ENSURE(supported,
                 QUARK_ERR(util::Errc::Unsupported,
                           "Required device extension unsupported {}", name));

    if (!std::ranges::contains(enabled, name)) {
      enabled.push_back(name);
      QUARK_LOG_INFO("device extension enabled: '{}'", name);
    }
    QUARK_OK();
  };

  for (const char *name : required) {
    QUARK_TRY_STATUS(request_one(name));
  }

#if defined(__APPLE__)
  // MoltenVK usually needs portability subset;
  constexpr const char *kPortabilitySubset = "VK_KHR_portability_subset";
  if (has_device_extension_props_(props, kPortabilitySubset) &&
      !std::ranges::contains(enabled, kPortabilitySubset)) {
    enabled.push_back(kPortabilitySubset);
    QUARK_LOG_INFO("portability subset: enabled");
  }
#endif

  return enabled;
}

util::Result<uint32_t>
Device::find_graphics_queue_family_or_error_(VkPhysicalDevice physical_device) {
  uint32_t queue_family_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           /*pQueueFamilyProperties=*/nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           queue_families.data());

  for (auto index{0UZ}; index < queue_family_count; ++index) {
    if ((queue_families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
      return index;
    }
  }

  QUARK_FAIL(
      QUARK_ERR(util::Errc::Unsupported, "No graphics queue family found"));
}

util::Result<uint32_t>
Device::find_present_queue_family_or_error_(VkPhysicalDevice physical_device,
                                            VkSurfaceKHR surface) {
  uint32_t queue_family_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           /*pQueueFamilyProperties=*/nullptr);

  for (auto index{0UZ}; index < queue_family_count; ++index) {
    VkBool32 present_supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface,
                                         &present_supported);
    if (present_supported == VK_TRUE) {
      return index;
    }
  }

  QUARK_FAIL(
      QUARK_ERR(util::Errc::Unsupported, "No present queue family found"));
}

} // namespace quark::vk::details
