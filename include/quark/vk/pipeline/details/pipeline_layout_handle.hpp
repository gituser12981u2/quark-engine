#pragma once

#include <quark/engine/handle/opaque_handle.hpp>

namespace quark::vk::details {

struct PipelineLayoutHandleTag;
using PipelineLayoutHandle = engine::GenericHandle<PipelineLayoutHandleTag>;

} // namespace quark::vk::details
