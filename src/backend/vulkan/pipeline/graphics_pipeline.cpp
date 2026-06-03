#include "quark/vk/pipeline/graphics_pipeline.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/diagnostic_prelude.hpp"
#include "quark/vk/pipeline/graphics_pipeline_desc.hpp"
#include "quark/vk/pipeline/shader_stage_desc.hpp"
#include "quark/vk/pipeline/vertex_layout.hpp"
#include "quark/vk/vk_error.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

namespace {

VkVertexInputBindingDescription to_vk_binding(const VertexBindingDesc &desc) {
  VkVertexInputBindingDescription out{};
  out.binding = desc.binding;
  out.stride = desc.stride;
  out.inputRate = desc.input_rate;
  return out;
}

VkVertexInputAttributeDescription
to_vk_attribute(const VertexAttributeDesc &desc) {
  VkVertexInputAttributeDescription out{};
  out.location = desc.location;
  out.binding = desc.binding;
  out.format = desc.format;
  out.offset = desc.offset;
  return out;
}

VkPipelineColorBlendAttachmentState
make_blend_attachment(const ColorAttachmentDesc &desc) {
  VkPipelineColorBlendAttachmentState out{};
  out.blendEnable = desc.blend_enable ? VK_TRUE : VK_FALSE;
  out.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  out.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  out.colorBlendOp = VK_BLEND_OP_ADD;
  out.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  out.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  out.alphaBlendOp = VK_BLEND_OP_ADD;
  out.colorWriteMask = desc.write_mask;
  return out;
}

} // namespace

util::Status GraphicsPipeline::create(const CreateInfo &ci) {
  QUARK_ENSURE(
      ci.device != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidArg, "graphics pipeline device is null"));
  QUARK_ENSURE(ci.desc != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                             "graphics pipeline desc is null"));

  const GraphicsPipelineDesc &desc = *ci.desc;

  QUARK_ENSURE(!desc.stages.empty(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has no shader stages"));
  QUARK_ENSURE(!desc.color_attachments.empty(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has no color attachments"));

  destroy();
  device_ = ci.device;

  std::vector<VkPipelineShaderStageCreateInfo> stages;
  stages.reserve(desc.stages.size());

  for (const ShaderStageDesc &stage_desc : desc.stages) {
    QUARK_ENSURE(stage_desc.module != VK_NULL_HANDLE,
                 QUARK_ERR(util::Errc::InvalidArg,
                           "graphics pipeline shader module is null"));
    QUARK_ENSURE(stage_desc.entry_point != nullptr,
                 QUARK_ERR(util::Errc::InvalidArg,
                           "graphics pipeline shader entry point is null"));

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = stage_desc.stage;
    stage.module = stage_desc.module;
    stage.pName = stage_desc.entry_point;
    stages.push_back(stage);
  }

  std::vector<VkVertexInputBindingDescription> bindings{};
  bindings.reserve(desc.vertex_layout.bindings.size());
  for (const VertexBindingDesc &binding : desc.vertex_layout.bindings) {
    bindings.push_back(to_vk_binding(binding));
  }

  std::vector<VkVertexInputAttributeDescription> attributes;
  attributes.reserve(desc.vertex_layout.attributes.size());
  for (const VertexAttributeDesc &attribute : desc.vertex_layout.attributes) {
    attributes.push_back(to_vk_attribute(attribute));
  }

  VkPipelineVertexInputStateCreateInfo vertex_input{};
  vertex_input.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindings.size());
  vertex_input.pVertexBindingDescriptions = bindings.data();
  vertex_input.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributes.size());
  vertex_input.pVertexAttributeDescriptions = attributes.data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = desc.topology;
  input_assembly.primitiveRestartEnable =
      desc.primitive_restart_enable ? VK_TRUE : VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(ci.extent.width);
  viewport.height = static_cast<float>(ci.extent.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;

  VkRect2D scissor{};
  scissor.offset = {.x = 0, .y = 0};
  scissor.extent = ci.extent;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable =
      desc.raster.depth_clamp_enable ? VK_TRUE : VK_FALSE;
  rasterizer.rasterizerDiscardEnable =
      desc.raster.rasterizer_discard_enable ? VK_TRUE : VK_FALSE;
  rasterizer.polygonMode = desc.raster.polygon_mode;
  rasterizer.cullMode = desc.raster.cull_mode;
  rasterizer.frontFace = desc.raster.front_face;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.lineWidth = desc.raster.line_width;

  VkPipelineMultisampleStateCreateInfo multisample{};
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = desc.multisample.samples;

  // TODO: add depth stencil

  std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
  color_blend_attachments.reserve(desc.color_attachments.size());
  for (const ColorAttachmentDesc &attachment : desc.color_attachments) {
    color_blend_attachments.push_back(make_blend_attachment(attachment));
  }

  VkPipelineColorBlendStateCreateInfo color_blend{};
  color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend.logicOpEnable = VK_FALSE;
  color_blend.attachmentCount =
      static_cast<uint32_t>(color_blend_attachments.size());
  color_blend.pAttachments = color_blend_attachments.data();

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount =
      static_cast<uint32_t>(desc.dynamic_states.size());
  dynamic_state.pDynamicStates = desc.dynamic_states.data();

  VkPipelineLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_info.setLayoutCount = 0;
  layout_info.pSetLayouts = nullptr;
  layout_info.pushConstantRangeCount = 0;
  layout_info.pPushConstantRanges = nullptr;

  QUARK_VK_TRY(
      vkCreatePipelineLayout(device_, &layout_info, nullptr, &layout_));

  std::vector<VkFormat> color_formats;
  color_formats.reserve(desc.color_attachments.size());
  for (const ColorAttachmentDesc &attachment : desc.color_attachments) {
    color_formats.push_back(attachment.format);
  }

  VkPipelineRenderingCreateInfo rendering_info{};
  rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  rendering_info.colorAttachmentCount =
      static_cast<uint32_t>(color_formats.size());
  rendering_info.pColorAttachmentFormats = color_formats.data();
  rendering_info.depthAttachmentFormat = desc.depth_attachment.format;
  rendering_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = static_cast<uint32_t>(stages.size());
  pipeline_info.pStages = stages.data();
  pipeline_info.pVertexInputState = &vertex_input;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisample;
  pipeline_info.pDepthStencilState = nullptr;
  pipeline_info.pColorBlendState = &color_blend;
  pipeline_info.pDynamicState =
      desc.dynamic_states.empty() ? nullptr : &dynamic_state;
  pipeline_info.layout = layout_;
  // pipeline_info.renderPass = VK_NULL_HANDLE; // dynamic rendering
  pipeline_info.subpass = desc.subpass;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.basePipelineIndex = -1;

  if (desc.backend == PipelineRenderBackend::DynamicRendering) {
    pipeline_info.pNext = &rendering_info;
    pipeline_info.renderPass = VK_NULL_HANDLE;
  } else {
    QUARK_ENSURE(
        desc.render_pass != VK_NULL_HANDLE,
        QUARK_ERR(util::Errc::InvalidArg,
                  "render-pass graphics pipeline has null render pass"));

    pipeline_info.pNext = nullptr;
    pipeline_info.renderPass = desc.render_pass;
  }

  const VkResult result = vkCreateGraphicsPipelines(
      device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_);

  if (result != VK_SUCCESS) {
    destroy();
    QUARK_FAIL(::quark::vk::vk_error(result, "vkCreateGraphicsPipelines"));
  }

  QUARK_OK();
}

void GraphicsPipeline::destroy() noexcept {
  if (device_ == VK_NULL_HANDLE) {
    pipeline_ = VK_NULL_HANDLE;
    layout_ = VK_NULL_HANDLE;
    return;
  }

  if (pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_, pipeline_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
  }

  if (layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_, layout_, nullptr);
    layout_ = VK_NULL_HANDLE;
  }

  device_ = VK_NULL_HANDLE;
}

} // namespace quark::vk
