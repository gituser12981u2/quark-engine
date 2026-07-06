#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/rhi/shader/shader_stage.hpp"
#include <quark/rhi/shader/shader_stage_desc.hpp>

#include <quark/vk/pipeline/vertex_layout.hpp>
#include <quark/vk/triangle_renderer.hpp>

#include <array>
#include <quark/utils/diagnostic.hpp>

#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status TriangleRenderer::create(const TriangleRenderer::CreateInfo &ci) {
  destroy();

  QUARK_ENSURE(
      ci.allocator != nullptr,
      QUARK_ERR(util::Errc::InvalidArg, "Triangle renderer allocator is null"));

  QUARK_TRY_STATUS(shader_registry_.create({.device = ci.device.device}));

  rhi::details::ShaderHandle vert_shader{};
  rhi::details::ShaderHandle frag_shader{};

  QUARK_TRY_ASSIGN(vert_shader,
                   shader_registry_.load_spv_file(
                       "src/backend/shaders/spv/triangle.vert.spv"));

  QUARK_TRY_ASSIGN(frag_shader,
                   shader_registry_.load_spv_file(
                       "src/backend/shaders/spv/triangle.frag.spv"));

  const auto bindings = Position2Color3Vertex::bindings();
  const auto attributes = Position2Color3Vertex::attributes();
  const VertexLayoutDesc vertex_layout{
      .bindings = bindings,
      .attributes = attributes,
  };

  const std::array<rhi::ShaderStageDesc, 2> stages{
      rhi::ShaderStageDesc{
          .shader = vert_shader,
          .stage = rhi::ShaderStage::Vertex,
          .entry_point = "main",
      },
      rhi::ShaderStageDesc{
          .shader = frag_shader,
          .stage = rhi::ShaderStage::Fragment,
          .entry_point = "main",
      },
  };

  const std::array<ColorAttachmentDesc, 1> color_attachments{
      ColorAttachmentDesc{
          .format = ci.color_format,
      }};

  // const std::array<rhi::DescriptorBindingDesc, 1> camera_bindings{
  //     rhi::DescriptorBindingDesc{
  //         .binding = 0,
  //         .type = rhi::DescriptorType::UniformBuffer,
  //         .count = 1,
  //         .stages =
  //             static_cast<rhi::ShaderStageFlags>(rhi::ShaderStage::Vertex),
  //     },
  // };

  // const rhi::DescriptorSetLayoutDesc camera_layout_desc{
  //     .bindings = camera_bindings,
  // };
  //
  // QUARK_TRY_STATUS(camera_set_layout_.create({
  //     .device = ci.device,
  //     .desc = camera_layout_desc,
  //     .allocator = nullptr,
  // }));

  // const std::array<const DescriptorSetLayout *, 1> set_layouts{
  //     &camera_set_layout_,
  // };

  const rhi::PipelineLayout::Desc pipeline_layout_desc{
      .descriptor_set_layouts = {},
      .push_constant_ranges = {},
  };

  QUARK_TRY_STATUS(pipeline_layout_.create({
      .device = ci.device,
      // .descriptor_set_layouts = nullptr,
      .retire_queue = ci.retire_queue,
      // .allocator = nullptr,
      .desc = &pipeline_layout_desc,
  }));

  const std::array<VkDynamicState, 2> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT,
                                                     VK_DYNAMIC_STATE_SCISSOR};

  const GraphicsPipelineDesc desc{
      .stages = stages,
      .vertex_layout = vertex_layout,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitive_restart_enable = false,
      .raster = RasterStateDesc{.cull_mode = VK_CULL_MODE_NONE,
                                .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE},
      .color_attachments = color_attachments,
      .dynamic_states = dynamic_states,
      .pipeline_layout = &pipeline_layout_,
      .backend = ci.backend,
      .render_pass = ci.render_pass,
  };

  QUARK_TRY_STATUS(pipeline_.create({
      .device = ci.device,
      .extent = ci.extent,
      .shaders = &shader_registry_,
      .retire_queue = ci.retire_queue,
      .desc = &desc,
  }));

  QUARK_TRY_STATUS(vertex_buffer_.create(*ci.allocator));
  QUARK_OK();
}

void TriangleRenderer::destroy() noexcept {
  vertex_buffer_.destroy();
  pipeline_.destroy();
  pipeline_layout_.destroy();
  shader_registry_.destroy();
}

void TriangleRenderer::draw(VkCommandBuffer command_buffer,
                            VkExtent2D extent) const noexcept {
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.view().vk_pipeline());

  VkViewport viewport{};
  viewport.x = 0.0F;
  viewport.y = 0.0F;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0F;
  viewport.maxDepth = 1.0F;
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {.x = 0, .y = 0};
  scissor.extent = extent;
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  vertex_buffer_.bind(command_buffer);
  vkCmdDraw(command_buffer, vertex_buffer_.vertex_count(), 1, 0, 0);
}

} // namespace quark::vk
