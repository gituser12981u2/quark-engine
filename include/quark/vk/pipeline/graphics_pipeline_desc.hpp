#pragma once

#include "quark/vk/pipeline/pipeline_layout.hpp"
#include "quark/vk/pipeline/shader_stage_desc.hpp"
#include "quark/vk/pipeline/vertex_layout.hpp"

#include <cstdint>
#include <span>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

// Probably get rid of this since its always binary
enum class PipelineRenderBackend : uint8_t {
  RenderPass,
  DynamicRendering,
};

struct ColorAttachmentDesc {
  VkFormat format{VK_FORMAT_UNDEFINED};
  bool blend_enable{false};

  VkColorComponentFlags write_mask{
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
};

struct DepthAttachmentDesc {
  VkFormat format{VK_FORMAT_UNDEFINED};
  bool test_enable{false};
  bool write_enable{false};
  VkCompareOp compare_op{VK_COMPARE_OP_LESS};
};

struct RasterStateDesc {
  VkPolygonMode polygon_mode{VK_POLYGON_MODE_FILL};
  VkCullModeFlags cull_mode{VK_CULL_MODE_BACK_BIT};
  VkFrontFace front_face{VK_FRONT_FACE_COUNTER_CLOCKWISE};
  bool depth_clamp_enable{false};
  bool rasterizer_discard_enable{false};
  float line_width{1.0F};
};

struct MultisampleStateDesc {
  VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
};

struct GraphicsPipelineDesc {
  std::span<const ShaderStageDesc> stages;

  VertexLayoutDesc vertex_layout{};
  VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
  bool primitive_restart_enable{false};

  RasterStateDesc raster{};
  MultisampleStateDesc multisample{};

  std::span<const ColorAttachmentDesc> color_attachments;
  DepthAttachmentDesc depth_attachment{};

  std::span<const VkDynamicState> dynamic_states;

  const PipelineLayout *pipeline_layout{nullptr};

  // Probably shouldn't default initialize to anything
  PipelineRenderBackend backend{PipelineRenderBackend::DynamicRendering};
  VkRenderPass render_pass{VK_NULL_HANDLE};
  uint32_t subpass{0};
};

} // namespace quark::vk
