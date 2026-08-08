#pragma once

#include "quark/engine/handle/opaque_handle.hpp"

namespace quark::rhi::details {

struct DeviceHandleTag;
using DeviceHandle = engine::GenericHandle<DeviceHandleTag>;

} // namespace quark::rhi::details
