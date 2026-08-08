#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

/**
 * @class VertexAttributeDesc
 * @brief Describes a single vertex attribute.
 *
 * Vertex attributes define how elements of a vertex are mapped to shader input
 * locations.
 */
struct VertexAttributeDesc {
  uint32_t location{};
  uint32_t binding{};
  VkFormat format{VK_FORMAT_UNDEFINED};
  uint32_t offset{};
};

/**
 * @class VertexBindingDesc
 * @brief Describes a vertex buffer binding.
 *
 * A binding specifies the memory layout and input rate for a vertex system.
 */
struct VertexBindingDesc {
  uint32_t binding{};
  uint32_t stride{};
  VkVertexInputRate input_rate{VK_VERTEX_INPUT_RATE_VERTEX};
};

/**
 * @class VertexLayoutDesc
 * @brief Describes the complete vertex input layout for a pipeline.
 *
 * The layout consists of one or more buffer bindings and the attributes sourced
 * from those bindings.
 *
 * The descriptor references externally owned arrays and does not take
 * ownership.
 */
struct VertexLayoutDesc {
  std::span<const VertexBindingDesc> bindings;
  std::span<const VertexAttributeDesc> attributes;
};

// TODO: make 3 positional
struct Position2Color3Vertex {
  float x{};
  float y{};
  float r{};
  float g{};
  float b{};

  static constexpr auto bindings() noexcept
      -> std::array<VertexBindingDesc, 1> {
    return {{
        VertexBindingDesc{
            .binding = 0,
            .stride = sizeof(Position2Color3Vertex),
            .input_rate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    }};
  }

  static constexpr auto attributes() noexcept
      -> std::array<VertexAttributeDesc, 2> {
    return {{
        VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 0,
        },
        VertexAttributeDesc{
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = sizeof(float) * 2,
        },
    }};
  }
};

} // namespace quark::vk
