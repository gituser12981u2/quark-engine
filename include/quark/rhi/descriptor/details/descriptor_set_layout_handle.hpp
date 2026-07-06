#pragma once

#include "quark/engine/handle/opaque_handle.hpp"

namespace quark::rhi::details {

struct DescriptorSetLayoutHandleTag;
using DescriptorSetLayoutHandle =
    engine::GenericHandle<DescriptorSetLayoutHandleTag>;

} // namespace quark::rhi::details
