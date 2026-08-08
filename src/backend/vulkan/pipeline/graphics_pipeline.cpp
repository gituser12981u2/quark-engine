#include <array>
#include <cstdint>
#include <quark/platform/shader/details/shader_registry.hpp>
#include <quark/utils/diagnostic.hpp>
#include <quark/utils/error_types.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/pipeline/details/graphics_pipeline.hpp>
#include <quark/vk/pipeline/graphics_pipeline_desc.hpp>
#include <quark/vk/pipeline/pipeline_limits.hpp>
#include <quark/vk/pipeline/shader_stage_desc.hpp>
#include <quark/vk/pipeline/vertex_layout.hpp>
#include <quark/vk/vk_error.hpp>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

namespace {

// TODO: replace with inplace_vector and upgrade to c++26
struct PipelineScratch {
  std::array<VkPipelineShaderStageCreateInfo,
             pipeline_limits::kMaxGraphicsShaderStages>
      stages{};

  std::array<VkShaderModule, pipeline_limits::kMaxGraphicsShaderStages>
      shader_modules{};

  std::array<VkVertexInputBindingDescription,
             pipeline_limits::kMaxVertexBindings>
      bindings{};

  std::array<VkVertexInputAttributeDescription,
             pipeline_limits::kMaxVertexAttributes>
      attributes{};

  std::array<VkPipelineColorBlendAttachmentState,
             pipeline_limits::kMaxColorAttachments>
      color_blend_attachments{};

  std::array<VkFormat, pipeline_limits::kMaxColorAttachments> color_formats{};

  uint32_t stage_count{};
  uint32_t binding_count{};
  uint32_t attribute_count{};
  uint32_t color_attachments_count{};
};

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

util::Status validate_desc(const GraphicsPipeline::CreateInfo &ci) {
  QUARK_ENSURE(
      ci.device.device != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidArg, "graphics pipeline device is null"));

  const GraphicsPipelineDesc &desc = *ci.desc;

  QUARK_ENSURE(!desc.color_attachments.empty(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has no color attachments"));

  QUARK_ENSURE(desc.stages.size() <= pipeline_limits::kMaxGraphicsShaderStages,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has too many shader stages"));

  QUARK_ENSURE(ci.shaders != nullptr,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline shader registry is null"));

  QUARK_ENSURE(desc.vertex_layout.bindings.size() <=
                   pipeline_limits::kMaxVertexBindings,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has too many vertex bindings"));

  QUARK_ENSURE(desc.vertex_layout.attributes.size() <=
                   pipeline_limits::kMaxVertexAttributes,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has too many vertex attributes"));

  QUARK_ENSURE(desc.color_attachments.size() <=
                   pipeline_limits::kMaxColorAttachments,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has too many color attachments"));

  QUARK_ENSURE(desc.dynamic_states.size() <= pipeline_limits::kMaxDynamicStates,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has too many dynamic states"));

  if (desc.backend == PipelineRenderBackend::RenderPass) {
    QUARK_ENSURE(
        desc.render_pass != VK_NULL_HANDLE,
        QUARK_ERR(util::Errc::InvalidArg,
                  "render-pass graphics pipeline has null render pass"));
  }

  QUARK_OK();
}

util::Status build_scratch(VkDevice device, const GraphicsPipelineDesc &desc,
                           const ShaderRegistry &shaders,
                           PipelineScratch &scratch) {
  for (const ShaderStageDesc &stage_desc : desc.stages) {
    QUARK_ENSURE(shaders.alive(stage_desc.shader),
                 QUARK_ERR(util::Errc::InvalidArg,
                           "graphics pipeline shader module is null"));
    QUARK_ENSURE(stage_desc.entry_point != nullptr,
                 QUARK_ERR(util::Errc::InvalidArg,
                           "graphics pipeline shader entry point is null"));

    const std::span<const uint32_t> code = shaders.code(stage_desc.shader);

    VkShaderModuleCreateInfo module_info{};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = code.size_bytes();
    module_info.pCode = code.data();

    VkShaderModule module{VK_NULL_HANDLE};
    QUARK_VK_TRY(vkCreateShaderModule(device, &module_info,
                                      /*pAllocator=*/nullptr, &module));

    scratch.shader_modules[scratch.stage_count] = module;

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = stage_desc.stage;
    stage.module = module;
    stage.pName = stage_desc.entry_point;

    scratch.stages[scratch.stage_count++] = stage;
  }

  for (const VertexBindingDesc &binding : desc.vertex_layout.bindings) {
    scratch.bindings[scratch.binding_count++] = to_vk_binding(binding);
  }

  for (const VertexAttributeDesc &attribute : desc.vertex_layout.attributes) {
    scratch.attributes[scratch.attribute_count++] = to_vk_attribute(attribute);
  }

  for (const ColorAttachmentDesc &attachment : desc.color_attachments) {
    scratch.color_blend_attachments[scratch.color_attachments_count] =
        make_blend_attachment(attachment);
    scratch.color_formats[scratch.color_attachments_count] = attachment.format;
    ++scratch.color_attachments_count;
  }

  QUARK_OK();
}

void destroy_scratch_shader_modules(VkDevice device,
                                    PipelineScratch &scratch) noexcept {
  if (device == VK_NULL_HANDLE) {
    return;
  }

  for (VkShaderModule &module : scratch.shader_modules) {
    if (module == VK_NULL_HANDLE) {
      continue;
    }

    vkDestroyShaderModule(device, module, nullptr);
    module = VK_NULL_HANDLE;
  }
}

} // namespace

util::Status GraphicsPipeline::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate_desc(ci));

  destroy();
  auto *device = ci.device.device;

  const GraphicsPipelineDesc &desc = *ci.desc;

  QUARK_ENSURE(!desc.stages.empty(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has no shader stages"));
  QUARK_ENSURE(!desc.color_attachments.empty(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline has no color attachments"));

  PipelineScratch scratch{};
  QUARK_TRY_STATUS(build_scratch(device, desc, *ci.shaders, scratch));

  VkPipelineVertexInputStateCreateInfo vertex_input{};
  vertex_input.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input.vertexBindingDescriptionCount = scratch.binding_count;
  vertex_input.pVertexBindingDescriptions = scratch.bindings.data();
  vertex_input.vertexAttributeDescriptionCount = scratch.attribute_count;
  vertex_input.pVertexAttributeDescriptions = scratch.attributes.data();

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
  scissor = {};
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

  VkPipelineColorBlendStateCreateInfo color_blend{};
  color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend.logicOpEnable = VK_FALSE;
  color_blend.attachmentCount = scratch.color_attachments_count;
  color_blend.pAttachments = scratch.color_blend_attachments.data();

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount =
      static_cast<uint32_t>(desc.dynamic_states.size());
  dynamic_state.pDynamicStates = desc.dynamic_states.data();

  QUARK_TRY_STATUS(layout_.create({
      .device = ci.device,
      .set_layout_count = 0,
      .set_layouts = nullptr,
      .push_constant_range_count = 0,
      .push_constant_ranges = nullptr,
      .allocator = ci.allocator,
  }));

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
  pipeline_info.stageCount = scratch.stage_count;
  pipeline_info.pStages = scratch.stages.data();
  pipeline_info.pVertexInputState = &vertex_input;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisample;
  pipeline_info.pDepthStencilState = nullptr;
  // desc.depth_attachment.format == VK_FORMAT_UNDEFINED ? nullptr
  //                                                     : &depth_stencil;
  pipeline_info.pColorBlendState = &color_blend;
  pipeline_info.pDynamicState =
      desc.dynamic_states.empty() ? nullptr : &dynamic_state;
  pipeline_info.layout = layout_.handle();
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

  QUARK_TRY_STATUS(pipeline_.create({
      .device = ci.device,
      .cache = VK_NULL_HANDLE,
      .graphics_info = &pipeline_info,
      .allocator = ci.allocator,
  }));

  destroy_scratch_shader_modules(device, scratch);

  QUARK_OK();
}

void GraphicsPipeline::destroy() noexcept {
  pipeline_.destroy();
  layout_.destroy();
}

} // namespace quark::vk::details
