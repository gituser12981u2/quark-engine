#pragma once

#include <cstdint>

namespace quark::rhi {

enum class ShaderStage : uint32_t {
  Vertex = 1U << 0U,
  Fragment = 1U << 1U,
  Compute = 1U << 2U,
};

using ShaderStageFlags = uint32_t;

constexpr ShaderStageFlags operator|(ShaderStage a, ShaderStage b) noexcept {
  return static_cast<ShaderStageFlags>(a) | static_cast<ShaderStageFlags>(b);
}

constexpr ShaderStageFlags operator|(ShaderStageFlags a,
                                     ShaderStage b) noexcept {
  return a | static_cast<ShaderStageFlags>(b);
}

} // namespace quark::rhi
