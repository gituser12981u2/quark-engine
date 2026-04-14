#pragma once

#include "quark/utils/result.hpp"
#include <memory_resource>
#include <quark/utils/allocator.hpp>
#include <quark/utils/raii.hpp>
#include <quark/vk/device/details/device.hpp>
#include <quark/vk/device/details/device_handle.hpp>
#include <quark/vk/device/details/device_registry.hpp>

namespace quark::vk {

class DeviceBundle final {
public:
  DeviceBundle() = default;
  explicit DeviceBundle(std::pmr::memory_resource *memory_resource);
  explicit DeviceBundle(const Device::CreateInfo &ci);
  DeviceBundle(const Device::CreateInfo &ci,
               std::pmr::memory_resource *memory_resource);
  ~DeviceBundle() { destroy(); }

  QUARK_MOVE_ONLY(DeviceBundle);

  util::Status create(const Device::CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return registry_.alive(handle_); }

  [[nodiscard]] DeviceHandle handle() const noexcept { return handle_; }

  [[nodiscard]] VkPhysicalDevice vk_physical_device() const noexcept;
  [[nodiscard]] VkDevice vk_device() const noexcept;
  [[nodiscard]] VkQueue graphics_queue() const noexcept;
  [[nodiscard]] VkQueue present_queue() const noexcept;
  [[nodiscard]] uint32_t graphics_queue_family_index() const noexcept;
  [[nodiscard]] uint32_t present_queue_family_index() const noexcept;

private:
  DeviceRegistry registry_;
  DeviceHandle handle_{};
};

} // namespace quark::vk
