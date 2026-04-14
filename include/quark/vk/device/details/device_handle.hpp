#pragma once

#include <quark/utils/generic_handle.hpp>

namespace quark::vk {

struct DeviceHandleTag;

using DeviceHandle = util::GenericHandle<DeviceHandleTag>;

} // namespace quark::vk
