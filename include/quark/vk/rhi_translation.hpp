#pragma once

#include "quark/rhi/descriptor/descriptor_set_layout_desc.hpp"

#include <vulkan/vulkan_core.h>

namespace quark::vk {

[[nodiscard]] VkDescriptorType
to_vk_descriptor_type(rhi::DescriptorType type) noexcept;

[[nodiscard]] VkShaderStageFlags
to_vk_shader_stages(rhi::ShaderStageFlags stages) noexcept;

} // namespace quark::vk
