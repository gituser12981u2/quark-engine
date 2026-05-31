#pragma once

#include <quark/engine/handle/opaque_handle.hpp>

namespace quark::vk::details {

struct FrameHandleTag;

using FrameHandle = engine::GenericHandle<FrameHandleTag>;

} // namespace quark::vk::details
