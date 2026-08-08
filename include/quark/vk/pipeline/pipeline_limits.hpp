#pragma once

#include <cstdint>

namespace quark::vk::pipeline_limits {

/**
 * @brief Internal scratch-buffer capacities used during pipeline creation.
 *
 * These limits bound temporary storage used while translating pipeline
 * descriptors into Vulkan pipeline state. Values shoudl be chosen to
 * comfortably cover expected renderer usage while avoding heap allocation
 * during pipeline construction.
 */

/// Maximum shader stages supported by a graphics pipeline descriptor.
inline constexpr uint32_t kMaxGraphicsShaderStages = 5;

/// Maximum vertex buffer bindings processed during a graphics pipeline
/// creation.
inline constexpr uint32_t kMaxVertexBindings = 8;

/// Maximum vertex attributes processed during a graphics pipeline
/// creation.
inline constexpr uint32_t kMaxVertexAttributes = 16;

/// Maximum color attachments processed during a graphics pipeline
/// creation.
inline constexpr uint32_t kMaxColorAttachments = 8;

/// Maximum dynamic state entries processed during a graphics pipeline
/// creation.
inline constexpr uint32_t kMaxDynamicStates = 16;

inline constexpr uint32_t kMaxDescriptorSetLayouts = 8;
inline constexpr uint32_t kMaxPushConstantRanges = 8;

} // namespace quark::vk::pipeline_limits
