#pragma once

#include <quark/engine/handle/opaque_handle.hpp>

namespace quark::vk::details {

struct GraphicsPipelineHandleTag;
using GraphicsPipelineHandle = engine::GenericHandle<GraphicsPipelineHandleTag>;

} // namespace quark::vk::details
