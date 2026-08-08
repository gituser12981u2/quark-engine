#pragma once

#include "quark/vk/device/device_view.hpp"
#include <vulkan/vulkan_core.h>

namespace quark::rhi {

namespace details {
class BackendAccess;
}

class DeviceView {
public:
  DeviceView() = default;

  explicit DeviceView(vk::DeviceView backend) noexcept : backend_(backend) {}

  [[nodiscard]] bool valid() const noexcept {
    return backend_.device != VK_NULL_HANDLE;
  }

  friend bool operator==(DeviceView lhs, DeviceView rhs) noexcept {
    return lhs.backend_.device = rhs.backend_.device;
  }

private:
  friend class details::BackendAccess;

  vk::DeviceView backend_{};
};

} // namespace quark::rhi
