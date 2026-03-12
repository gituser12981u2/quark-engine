#pragma once

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

class Device final {
public:
  // NOLINTNEXTLINE(performance-enum-size)
  enum class Features : uint64_t {
    TimelineSemaphore = 1ULL << 0,
  };
  using FeatureFlags = uint64_t;

  struct CreateInfo {
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkAllocationCallbacks *allocator = nullptr;

    std::vector<const char *> required_extensions;

    FeatureFlags requested_features = 0;
  };

  Device() = default;
  ~Device() { destroy(); }

  QUARK_MOVE_ONLY(Device);

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return device_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPhysicalDevice vk_physical_device() const noexcept {
    return physical_device_;
  }

  [[nodiscard]] VkDevice vk_device() const noexcept { return device_; }

  [[nodiscard]] VkQueue graphics_queue() const noexcept {
    return graphics_queue_;
  }

  [[nodiscard]] VkQueue present_queue() const noexcept {
    return present_queue_;
  }

  [[nodiscard]] uint32_t graphics_queue_family_index() const noexcept {
    return graphics_queue_family_index_;
  }

  [[nodiscard]] uint32_t present_queue_family_index() const noexcept {
    return present_queue_family_index_;
  }

private:
  struct DeviceSelection {
    VkPhysicalDevice physical_device{VK_NULL_HANDLE};
    uint32_t graphics_queue_family_index{0};
    uint32_t present_queue_family_index{0};
  };

  static util::Result<DeviceSelection>
  pick_physical_device_(VkInstance instance, VkSurfaceKHR surface,
                        const std::vector<const char *> &required_extensions);

  static util::Result<std::vector<VkExtensionProperties>>
  enumerate_device_extensions_(VkPhysicalDevice physical_device);

  static bool
  has_device_extension_props_(const std::vector<VkExtensionProperties> &props,
                              const char *extension_name) noexcept;

  static util::Result<std::vector<const char *>>
  build_device_extensions_(VkPhysicalDevice physical_device,
                           const std::vector<const char *> &required);

  static util::Result<uint32_t>
  find_graphics_queue_family_or_error_(VkPhysicalDevice physical_device);

  static util::Result<uint32_t>
  find_present_queue_family_or_error_(VkPhysicalDevice physical_device,
                                      VkSurfaceKHR surface);

  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;
  uint32_t graphics_queue_family_index_ = 0;
  uint32_t present_queue_family_index_ = 0;
  const VkAllocationCallbacks *alloc_ = nullptr;
};

} // namespace quark::vk::details
