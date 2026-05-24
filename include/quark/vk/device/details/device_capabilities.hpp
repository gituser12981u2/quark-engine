#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

// NOLINTNEXTLINE(performance-enum-size)
enum class DeviceFeature : uint64_t {
  TimelineSemaphore = 1ULL << 0,
  DynamicRendering = 1ULL << 1,
  Synchronization2 = 1ULL << 2,
};

using DeviceFeatureFlags = uint64_t;

/**
 * @class DeviceCapabilities
 * @brief Engine-visible summary of physical-device feature support and
 * enablement.
 *
 * supported_features records what the queried physical device/runtime reports
 * as available. enabled_features records what this engine actually requested
 * and enabled when creating the logical device.
 *
 */
struct DeviceCapabilities {
  uint32_t api_version = VK_API_VERSION_1_0;
  DeviceFeatureFlags supported_features = 0;
  DeviceFeatureFlags enabled_features = 0;

  [[nodiscard]] bool api_at_least(uint32_t major,
                                  uint32_t minor) const noexcept {
    const auto vmaj = VK_VERSION_MAJOR(api_version);
    const auto vmin = VK_VERSION_MINOR(api_version);
    return (vmaj > major) || (vmaj == major && vmin >= minor);
  }

  [[nodiscard]] bool supports(DeviceFeature feature) const noexcept {
    return (supported_features & static_cast<DeviceFeatureFlags>(feature)) != 0;
  }

  [[nodiscard]] bool enabled(DeviceFeature feature) const noexcept {
    return (enabled_features & static_cast<DeviceFeatureFlags>(feature)) != 0;
  }

  [[nodiscard]] bool is_vk12_or_newer() const noexcept {
    return api_at_least(1, 2);
  }

  [[nodiscard]] bool is_vk13_or_newer() const noexcept {
    return api_at_least(1, 3);
  }
};

} // namespace quark::vk::details
