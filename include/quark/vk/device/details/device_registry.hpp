#pragma once

#include "device.hpp"
#include "device_handle.hpp"

#include <quark/utils/generational_registry.hpp>

namespace quark::vk {

using DeviceRegistry = util::GenerationalRegistry<Device, DeviceHandle>;

} // namespace quark::vk
