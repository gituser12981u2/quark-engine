#include "quark/vk/device/device_view.hpp"
#include "quark/rhi/device/backend_access.hpp"
#include "quark/rhi/device/device_view.hpp"

namespace quark::rhi {

vk::DeviceView
details::BackendAccess::native_device(DeviceView device) noexcept {
  return device.backend_;
}

} // namespace quark::rhi
