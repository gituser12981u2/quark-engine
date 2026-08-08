#pragma once

#include <cstdint>

namespace quark::rhi {

enum class Format : uint16_t {
  Undefined,

  R8Unorm,
  R8Snorm,
  R8Uint,
  R8Sint,

  R8G8Unorm,
  R8G8Snorm,
  R8G8Uint,
  R8G8Sint,

  R8G8B8A8Unorm,
  R8G8B8A8Snorm,
  R8G8B8A8Uint,
  R8G8B8A8Sint,
  R8G8B8A8Srgb,

  B8G8R8A8Unorm,
  B8G8R8A8Srgb,

  R16Float,
  R16G16Float,
  R16G16B16A16Float,

  R32Float,
  R32G32Float,
  R32G32B32Float,
  R32G32B32A32Float,

  R32Uint,
  R32G32Uint,
  R32G32B32Uint,
  R32G32B32A32Uint,

  R32Sint,
  R32G32Sint,
  R32G32B32Sint,
  R32G32B32A32Sint,

  D16Unorm,
  D24UnormS8Uint,
  D32Float,
  D32FloatS8Uint,
};

} // namespace quark::rhi
