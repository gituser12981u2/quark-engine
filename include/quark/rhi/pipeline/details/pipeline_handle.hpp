#pragma once

#include <quark/engine/handle/opaque_handle.hpp>

namespace quark::rhi::details {

struct PipelineHandleTag;
using PipelineHandle = engine::GenericHandle<PipelineHandleTag>;

} // namespace quark::rhi::details
