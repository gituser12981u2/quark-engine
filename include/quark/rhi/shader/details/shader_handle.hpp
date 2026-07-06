#pragma once

#include "quark/engine/handle/opaque_handle.hpp"

namespace quark::rhi::details {

struct ShaderTag;
using ShaderHandle = engine::GenericHandle<ShaderTag>;

} // namespace quark::rhi::details
