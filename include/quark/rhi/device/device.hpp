#pragma once

#include "quark/rhi/device/device_view.hpp"

namespace quark::rhi {

class Device {
public:
  [[nodiscard]] DeviceView view() const noexcept { return DeviceView{handle_}; }
};

} // namespace quark::rhi
