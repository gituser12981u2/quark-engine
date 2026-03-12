#pragma once

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/details/device_registry.hpp>

namespace quark::vk {

namespace details {

class Device;
struct DeviceHandle;

} // namespace details

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

  [[nodiscard]] details::DeviceHandle handle() const noexcept {
    return handle_;
  }
  [[nodiscard]] DeviceView view() const noexcept;

  [[nodiscard]] VkPhysicalDevice vk_physical_device() const noexcept;
  [[nodiscard]] VkDevice vk_device() const noexcept;
  [[nodiscard]] VkQueue graphics_queue() const noexcept;
  [[nodiscard]] VkQueue present_queue() const noexcept;
  [[nodiscard]] uint32_t graphics_queue_family_index() const noexcept;
  [[nodiscard]] uint32_t present_queue_family_index() const noexcept;

private:
  details::DeviceRegistry registry_;
  details::DeviceHandle handle_{};
};

} // namespace quark::vk
