#pragma once

#include "quark/engine/handle/opaque_handle.hpp"

namespace quark::vk::details {

struct DescriptorSetLayoutHandleTag;
using DescriptorSetLayoutHandle =
    engine::GenericHandle<DescriptorSetLayoutHandleTag>;

} // namespace quark::vk::details
