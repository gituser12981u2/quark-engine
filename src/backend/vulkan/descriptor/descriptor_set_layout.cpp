#include "quark/rhi/descriptor/descriptor_set_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/vk/device/device_view.hpp"
#include "quark/vk/diagnostic_prelude.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

namespace {

[[nodiscard]] VkDescriptorType
to_vk_descriptor_type(rhi::DescriptorType type) noexcept {
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

[[nodiscard]] VkShaderStageFlags
to_vk_shader_stages(rhi::ShaderStageFlags stages) noexcept {
  VkShaderStageFlags flags{};

  if ((stages & static_cast<uint32_t>(rhi::ShaderStage::Vertex)) != 0U) {
    flags |= VK_SHADER_STAGE_VERTEX_BIT;
  }

  if ((stages & static_cast<uint32_t>(rhi::ShaderStage::Fragment)) != 0U) {
    flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  if ((stages & static_cast<uint32_t>(rhi::ShaderStage::Compute)) != 0U) {
    flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  return flags;
}

} // namespace

util::Status DescriptorSetLayoutBackend::create(const CreateInfo &ci) {
  QUARK_ENSURE(
      ci.desc != nullptr,
      QUARK_ERR(util::Errc::InvalidArg, "descriptor set layout desc is null"));

  QUARK_TRY_STATUS(validate(ci.device));

  destroy();

  device_ = ci.device;

  std::vector<VkDescriptorSetLayoutBinding> bindings;
  bindings.reserve(ci.desc->bindings.size());

  for (const rhi::DescriptorBindingDesc &binding : ci.desc->bindings) {
    const VkDescriptorType type = to_vk_descriptor_type(binding.type);
    const VkShaderStageFlags stages = to_vk_shader_stages(binding.stages);

    QUARK_ENSURE(
        type != VK_DESCRIPTOR_TYPE_MAX_ENUM,
        QUARK_ERR(util::Errc::InvalidArg, "invalid descriptor binding type"));

    QUARK_ENSURE(
        binding.count > 0,
        QUARK_ERR(util::Errc::InvalidArg, "descriptor binding count is zero"));

    QUARK_ENSURE(stages != 0,
                 QUARK_ERR(util::Errc::InvalidArg,
                           "descriptor binding shader stages are empty"));

    bindings.push_back(VkDescriptorSetLayoutBinding{
        .binding = binding.binding,
        .descriptorType = type,
        .descriptorCount = binding.count,
        .stageFlags = stages,
        .pImmutableSamplers = nullptr,
    });
  }

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.flags = 0;
  layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
  layout_info.pBindings = bindings.data();

  QUARK_VK_TRY(vkCreateDescriptorSetLayout(device_.device, &layout_info,
                                           /*pAllocator=*/nullptr, &handle_));

  QUARK_OK();
}

void DescriptorSetLayoutBackend::destroy() noexcept {
  if (device_.device != VK_NULL_HANDLE && handle_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_.device, handle_,
                                 /*pAllocator=*/nullptr);
  }

  handle_ = VK_NULL_HANDLE;
  device_ = {};
}

} // namespace quark::vk
