#pragma once

#include <quark/engine/handle/opaque_handle.hpp>

namespace quark::rhi::details {

struct PipelineLayoutHandleTag;
using PipelineLayoutHandle = engine::GenericHandle<PipelineLayoutHandleTag>;

} // namespace quark::rhi::details
