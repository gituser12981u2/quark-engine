#pragma once

#include "quark/rhi/shader/shader_stage.hpp"

#include <cstdint>
#include <span>

namespace quark::rhi {

enum class DescriptorType : uint8_t {
  UniformBuffer,
  StorageBuffer,
  CombinedImageSampler,
  SampledImage,
  StorageImage,
  Sampler,
};

struct DescriptorBindingDesc {
  uint32_t binding{};
  DescriptorType type{};
  uint32_t count{1};
  ShaderStageFlags stages{};
};

struct DescriptorSetLayoutDesc {
  std::span<const DescriptorBindingDesc> bindings;
};

} // namespace quark::rhi
