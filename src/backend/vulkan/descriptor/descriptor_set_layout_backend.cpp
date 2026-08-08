#include "quark/vk/descriptor/descriptor_set_layout_backend.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/vk/device/device_view.hpp"
#include "quark/vk/diagnostic_prelude.hpp"

#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status DescriptorSetLayoutBackend::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate(ci.device));

  QUARK_ENSURE(
      ci.desc != nullptr,
      QUARK_ERR(util::Errc::InvalidArg, "descriptor set layout desc is null"));

  std::vector<VkDescriptorSetLayoutBinding> bindings;
  bindings.reserve(ci.desc->bindings.size());

  for (const rhi::DescriptorBindingDesc &binding : ci.desc->bindings) {
    QUARK_ENSURE(
        binding.count > 0,
        QUARK_ERR(util::Errc::InvalidArg, "descriptor binding count is zero"));

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
  layout_info.pBindings = bindings.empty() ? nullptr : bindings.data();

  VkDescriptorSetLayout new_handle{VK_NULL_HANDLE};

  QUARK_VK_TRY(vkCreateDescriptorSetLayout(device_.device, &layout_info,
                                           ci.allocator, &new_handle));

  destroy();

  device_ = ci.device;
  handle_ = new_handle;
  allocator_ = ci.allocator;

  QUARK_OK();
}

void DescriptorSetLayoutBackend::destroy() noexcept {
  if (device_.device != VK_NULL_HANDLE && handle_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_.device, handle_, allocator_);
  }

  device_ = {};
  handle_ = VK_NULL_HANDLE;
  allocator_ = nullptr;
}

} // namespace quark::vk
