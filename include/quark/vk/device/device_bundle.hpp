#pragma once

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/details/device.hpp>
#include <quark/vk/device/details/device_capabilities.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

struct DeviceView;

class DeviceBundle final {
public:
  struct CreateInfo {
    details::Device::CreateInfo device{};
  };

  DeviceBundle() = default;
  explicit DeviceBundle(const DeviceBundle::CreateInfo &ci);
  ~DeviceBundle() { destroy(); }

  QUARK_MOVE_ONLY(DeviceBundle);

  util::Status create(const DeviceBundle::CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] util::Status validate() const noexcept;
  [[nodiscard]] DeviceView view() const noexcept;

  [[nodiscard]] VkPhysicalDevice vk_physical_device() const noexcept;
  [[nodiscard]] VkDevice vk_device() const noexcept;
  [[nodiscard]] VkQueue graphics_queue() const noexcept;
  [[nodiscard]] VkQueue present_queue() const noexcept;
  [[nodiscard]] uint32_t graphics_queue_family_index() const noexcept;
  [[nodiscard]] uint32_t present_queue_family_index() const noexcept;
  [[nodiscard]] details::DeviceCapabilities capabilities() const noexcept;
  [[nodiscard]] VmaAllocator vma_allocator() const noexcept;

private:
  details::Device device_;
};

} // namespace quark::vk
