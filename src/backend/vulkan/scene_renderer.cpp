#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

#include <quark/utils/diagnostic.hpp>
#include <quark/vk/pipeline/shader_stage_desc.hpp>
#include <quark/vk/scene_renderer.hpp>

namespace quark::vk {
namespace {

struct Vec3 {
  float x;
  float y;
  float z;
};

struct Mat4 {
  std::array<float, 16> values{};

  float &at(size_t row, size_t column) { return values[(column * 4) + row]; }
  [[nodiscard]] float at(size_t row, size_t column) const {
    return values[(column * 4) + row];
  }
};

struct ObjectPushConstants {
  Mat4 mvp;
  std::array<float, 4> tint{};
};

static_assert(sizeof(ObjectPushConstants) == 80);

Mat4 multiply(const Mat4 &left, const Mat4 &right) {
  Mat4 result{};
  for (size_t column = 0; column < 4; ++column) {
    for (size_t row = 0; row < 4; ++row) {
      for (size_t inner = 0; inner < 4; ++inner) {
        result.at(row, column) += left.at(row, inner) * right.at(inner, column);
      }
    }
  }
  return result;
}

Mat4 translation(float x, float y, float z) {
  Mat4 result{};
  result.at(0, 0) = 1.0F;
  result.at(1, 1) = 1.0F;
  result.at(2, 2) = 1.0F;
  result.at(3, 3) = 1.0F;
  result.at(0, 3) = x;
  result.at(1, 3) = y;
  result.at(2, 3) = z;
  return result;
}

Mat4 scale(float x, float y, float z) {
  Mat4 result{};
  result.at(0, 0) = x;
  result.at(1, 1) = y;
  result.at(2, 2) = z;
  result.at(3, 3) = 1.0F;
  return result;
}

Mat4 rotation_x(float angle) {
  Mat4 result{};
  const float cosine = std::cos(angle);
  const float sine = std::sin(angle);
  result.at(0, 0) = 1.0F;
  result.at(1, 1) = cosine;
  result.at(1, 2) = -sine;
  result.at(2, 1) = sine;
  result.at(2, 2) = cosine;
  result.at(3, 3) = 1.0F;
  return result;
}

Mat4 rotation_y(float angle) {
  Mat4 result{};
  const float cosine = std::cos(angle);
  const float sine = std::sin(angle);
  result.at(0, 0) = cosine;
  result.at(0, 2) = sine;
  result.at(1, 1) = 1.0F;
  result.at(2, 0) = -sine;
  result.at(2, 2) = cosine;
  result.at(3, 3) = 1.0F;
  return result;
}

Mat4 rotation_z(float angle) {
  Mat4 result{};
  const float cosine = std::cos(angle);
  const float sine = std::sin(angle);
  result.at(0, 0) = cosine;
  result.at(0, 1) = -sine;
  result.at(1, 0) = sine;
  result.at(1, 1) = cosine;
  result.at(2, 2) = 1.0F;
  result.at(3, 3) = 1.0F;
  return result;
}

Vec3 subtract(Vec3 a, Vec3 b) {
  return {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

Vec3 normalize(Vec3 value) {
  const float length = std::sqrt((value.x * value.x) + (value.y * value.y) +
                                 (value.z * value.z));
  return {.x = value.x / length, .y = value.y / length, .z = value.z / length};
}

Vec3 cross(Vec3 a, Vec3 b) {
  return {
      .x = (a.y * b.z) - (a.z * b.y),
      .y = (a.z * b.x) - (a.x * b.z),
      .z = (a.x * b.y) - (a.y * b.x),
  };
}

float dot(Vec3 a, Vec3 b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }

Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) {
  const Vec3 forward = normalize(subtract(target, eye));
  const Vec3 right = normalize(cross(forward, up));
  const Vec3 camera_up = cross(right, forward);

  Mat4 result{};
  result.at(0, 0) = right.x;
  result.at(0, 1) = right.y;
  result.at(0, 2) = right.z;
  result.at(0, 3) = -dot(right, eye);
  result.at(1, 0) = camera_up.x;
  result.at(1, 1) = camera_up.y;
  result.at(1, 2) = camera_up.z;
  result.at(1, 3) = -dot(camera_up, eye);
  result.at(2, 0) = -forward.x;
  result.at(2, 1) = -forward.y;
  result.at(2, 2) = -forward.z;
  result.at(2, 3) = dot(forward, eye);
  result.at(3, 3) = 1.0F;
  return result;
}

Mat4 perspective(float aspect_ratio) {
  constexpr float near_plane = 0.1F;
  constexpr float far_plane = 100.0F;
  const float focal_length = 1.0F / std::tan(std::numbers::pi_v<float> / 8.0F);

  Mat4 result{};
  result.at(0, 0) = focal_length / aspect_ratio;
  result.at(1, 1) = -focal_length;
  result.at(2, 2) = far_plane / (near_plane - far_plane);
  result.at(2, 3) = (far_plane * near_plane) / (near_plane - far_plane);
  result.at(3, 2) = -1.0F;
  return result;
}

std::array<MeshVertex, 24> make_cube_vertices() {
  constexpr std::array<std::array<float, 3>, 6> face_colors{
      {
          {1.0F, 0.34F, 0.28F},
          {0.22F, 0.78F, 0.38F},
          {0.28F, 0.48F, 1.0F},
          {1.0F, 0.78F, 0.22F},
          {0.75F, 0.32F, 0.92F},
          {0.22F, 0.82F, 0.88F},
      },
  };
  constexpr std::array<std::array<float, 3>, 8> corners{
      {
          {-0.5F, -0.5F, -0.5F},
          {0.5F, -0.5F, -0.5F},
          {0.5F, 0.5F, -0.5F},
          {-0.5F, 0.5F, -0.5F},
          {-0.5F, -0.5F, 0.5F},
          {0.5F, -0.5F, 0.5F},
          {0.5F, 0.5F, 0.5F},
          {-0.5F, 0.5F, 0.5F},
      },
  };
  constexpr std::array<std::array<uint8_t, 4>, 6> faces{
      {
          {0, 3, 2, 1},
          {4, 5, 6, 7},
          {0, 4, 7, 3},
          {1, 2, 6, 5},
          {0, 1, 5, 4},
          {3, 7, 6, 2},
      },
  };

  std::array<MeshVertex, 24> vertices{};
  size_t vertex_index = 0;
  for (size_t face_index = 0; face_index < faces.size(); ++face_index) {
    for (const uint8_t corner_index : faces[face_index]) {
      vertices[vertex_index++] = {
          .position =
              {
                  corners[corner_index][0],
                  corners[corner_index][1],
                  corners[corner_index][2],
              },
          .color =
              {
                  face_colors[face_index][0],
                  face_colors[face_index][1],
                  face_colors[face_index][2],
              },
      };
    }
  }
  return vertices;
}

constexpr std::array<uint32_t, 36> kCubeIndices{
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
};

} // namespace

util::Status SceneRenderer::create(const CreateInfo &ci) {
  destroy();
  QUARK_ENSURE(
      ci.allocator != nullptr,
      QUARK_ERR(util::Errc::InvalidArg, "scene renderer allocator is null"));

  QUARK_TRY_STATUS(shader_registry_.create({.device = ci.device.device}));
  ShaderHandle vertex_shader{};
  ShaderHandle fragment_shader{};
  QUARK_TRY_ASSIGN(vertex_shader, shader_registry_.load_spv_file(
                                      "src/backend/shaders/spv/cube.vert.spv",
                                      VK_SHADER_STAGE_VERTEX_BIT));
  QUARK_TRY_ASSIGN(fragment_shader, shader_registry_.load_spv_file(
                                        "src/backend/shaders/spv/cube.frag.spv",
                                        VK_SHADER_STAGE_FRAGMENT_BIT));

  constexpr std::array<VertexBindingDesc, 1> bindings{
      {
          {
              .binding = 0,
              .stride = sizeof(MeshVertex),
              .input_rate = VK_VERTEX_INPUT_RATE_VERTEX,
          },
      },
  };
  constexpr std::array<VertexAttributeDesc, 2> attributes{
      {
          {
              .location = 0,
              .binding = 0,
              .format = VK_FORMAT_R32G32B32_SFLOAT,
              .offset = offsetof(MeshVertex, position),
          },
          {
              .location = 1,
              .binding = 0,
              .format = VK_FORMAT_R32G32B32_SFLOAT,
              .offset = offsetof(MeshVertex, color),
          },
      },
  };
  const VertexLayoutDesc vertex_layout{
      .bindings = bindings,
      .attributes = attributes,
  };
  const std::array<ShaderStageDesc, 2> stages{
      {
          {
              .shader = vertex_shader,
              .stage = VK_SHADER_STAGE_VERTEX_BIT,
              .entry_point = "main",
          },
          {
              .shader = fragment_shader,
              .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
              .entry_point = "main",
          },
      },
  };
  const std::array<ColorAttachmentDesc, 1> color_attachments{
      {
          {.format = ci.color_format},
      },
  };
  const std::array<VkDynamicState, 2> dynamic_states{
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  const VkPushConstantRange push_constant_range{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = sizeof(ObjectPushConstants),
  };
  const GraphicsPipelineDesc desc{
      .stages = stages,
      .vertex_layout = vertex_layout,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .raster =
          {
              .cull_mode = VK_CULL_MODE_NONE,
              .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
          },
      .color_attachments = color_attachments,
      .depth_attachment =
          {
              .format = ci.depth_format,
              .test_enable = true,
              .write_enable = true,
              .compare_op = VK_COMPARE_OP_LESS,
          },
      .dynamic_states = dynamic_states,
      .push_constant_ranges = std::span(&push_constant_range, 1),
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

  const auto vertices = make_cube_vertices();
  QUARK_TRY_STATUS(cube_.create(*ci.allocator, vertices, kCubeIndices));
  QUARK_OK();
}

void SceneRenderer::destroy() noexcept {
  cube_.destroy();
  pipeline_.destroy();
  shader_registry_.destroy();
}

void SceneRenderer::draw(
    VkCommandBuffer command_buffer, VkExtent2D extent,
    std::chrono::steady_clock::duration elapsed) const noexcept {
  if (extent.width == 0 || extent.height == 0) {
    return;
  }

  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.pipeline());
  VkViewport viewport{
      .x = 0.0F,
      .y = 0.0F,
      .width = static_cast<float>(extent.width),
      .height = static_cast<float>(extent.height),
      .minDepth = 0.0F,
      .maxDepth = 1.0F,
  };
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  const VkRect2D scissor{.offset = {.x = 0, .y = 0}, .extent = extent};
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  const float seconds = std::chrono::duration<float>(elapsed).count();
  const Mat4 view = look_at({.x = 0.0F, .y = 0.0F, .z = 6.4F},
                            {.x = 0.0F, .y = 0.0F, .z = 0.0F},
                            {.x = 0.0F, .y = 1.0F, .z = 0.0F});
  const Mat4 view_projection =
      multiply(perspective(static_cast<float>(extent.width) /
                           static_cast<float>(extent.height)),
               view);

  for (size_t index = 0; index < objects_.size(); ++index) {
    const Object &object = objects_[index];
    const float spin = seconds * (0.35F + (static_cast<float>(index) * 0.1F));
    const Mat4 model = multiply(
        translation(object.position[0], object.position[1], object.position[2]),
        multiply(rotation_y(object.rotation[1] + spin),
                 multiply(rotation_x(object.rotation[0] + (spin * 0.7F)),
                          multiply(rotation_z(object.rotation[2]),
                                   scale(object.scale[0], object.scale[1],
                                         object.scale[2])))));
    const ObjectPushConstants constants{
        .mvp = multiply(view_projection, model),
        .tint = object.tint,
    };
    vkCmdPushConstants(command_buffer, pipeline_.layout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constants),
                       &constants);
    cube_.draw(command_buffer);
  }
}

} // namespace quark::vk
