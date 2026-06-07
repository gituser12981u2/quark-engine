#include <array>

#include <quark/utils/diagnostic.hpp>
#include <quark/vk/pipeline/shader_stage_desc.hpp>
#include <quark/vk/pipeline/vertex_layout.hpp>
#include <quark/vk/triangle_renderer.hpp>


namespace quark::vk {

util::Status TriangleRenderer::create(const TriangleRenderer::CreateInfo &ci) {
  destroy();

  QUARK_TRY_STATUS(shader_registry_.create({.device = ci.device.device}));

  ShaderHandle vert_shader{};
  ShaderHandle frag_shader{};

  QUARK_TRY_ASSIGN(vert_shader, shader_registry_.load_spv_file(
                                    "src/backend/shaders/spv/triangle.vert.spv",
                                    VK_SHADER_STAGE_VERTEX_BIT));

  QUARK_TRY_ASSIGN(frag_shader, shader_registry_.load_spv_file(
                                    "src/backend/shaders/spv/triangle.frag.spv",
                                    VK_SHADER_STAGE_FRAGMENT_BIT));

  const auto bindings = Position2Color3Vertex::bindings();
  const auto attributes = Position2Color3Vertex::attributes();
  const VertexLayoutDesc vertex_layout{
      .bindings = bindings,
      .attributes = attributes,
  };

  const std::array<ShaderStageDesc, 2> stages{
      ShaderStageDesc{
          .shader = vert_shader,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .entry_point = "main",
      },
      ShaderStageDesc{
          .shader = frag_shader,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .entry_point = "main",
      },
  };

  const std::array<ColorAttachmentDesc, 1> color_attachments{ColorAttachmentDesc{
      .format = ci.color_format,
  }};

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
      .backend = ci.backend,
      .render_pass = ci.render_pass,
  };

  QUARK_TRY_STATUS(pipeline_.create({
      .device = ci.device,
      .shaders = &shader_registry_,
      .retire_queue = ci.retire_queue,
      .extent = ci.extent,
      .desc = &desc,
  }));

  QUARK_TRY_STATUS(vertex_buffer_.create(ci.allocator));
  QUARK_OK();
}

void TriangleRenderer::destroy() noexcept {
  vertex_buffer_.destroy();
  pipeline_.destroy();
  shader_registry_.destroy();
}

void TriangleRenderer::draw(VkCommandBuffer command_buffer,
                            VkExtent2D extent) const noexcept {
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.pipeline());

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