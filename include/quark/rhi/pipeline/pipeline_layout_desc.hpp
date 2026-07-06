#pragma once

#include "quark/rhi/descriptor/descriptor_set_layout.hpp"

namespace quark::rhi {

struct PushConstantRange {
  ShaderStageFlags stages{};
  uint32_t offset{};
  uint32_t size{};
};

struct PipelineLayoutDesc {
  std::span<const DescriptorSetLayout *const> descriptor_set_layouts;
  std::span<const PushConstantRange> push_constant_ranges;
};

} // namespace quark::rhi
