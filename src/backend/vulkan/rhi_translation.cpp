#include "quark/vk/rhi_translation.hpp"

#include <cstdint>

namespace quark::vk {

VkDescriptorType to_vk_descriptor_type(rhi::DescriptorType type) noexcept {
  switch (type) {
  case rhi::DescriptorType::UniformBuffer:
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  case rhi::DescriptorType::StorageBuffer:
    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  case rhi::DescriptorType::CombinedImageSampler:
    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  case rhi::DescriptorType::SampledImage:
    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  case rhi::DescriptorType::StorageImage:
    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  case rhi::DescriptorType::Sampler:
    return VK_DESCRIPTOR_TYPE_SAMPLER;
  }

  return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

VkShaderStageFlags to_vk_shader_stages(rhi::ShaderStageFlags stages) noexcept {
  VkShaderStageFlags flags{};

  if ((stages & static_cast<uint32_t>(rhi::ShaderStage::Vertex)) != 0U) {
    flags |= VK_SHADER_STAGE_VERTEX_BIT;
  }

  if ((stages & static_cast<uint32_t>(rhi::ShaderStage::Fragment)) != 0U) {
    flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  if ((stages & static_cast<uint32_t>(rhi::ShaderStage::Compute)) != 0U) {
    flags |= VK_SHADER_STAGE_COMPUTE_BIT;
  }

  return flags;
}

} // namespace quark::vk
