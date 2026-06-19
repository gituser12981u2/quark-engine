#include "quark/vk/descriptor/descriptor_set_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"
#include "quark/vk/diagnostic_prelude.hpp"

#include <array>
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

namespace {

constexpr uint32_t kMaxDescriptorBindings = 32;

VkDescriptorSetLayoutBinding to_vk_bindings(const DescriptorBindingDesc &desc) {
  VkDescriptorSetLayoutBinding out{};
  out.binding = desc.binding;
  out.descriptorType = desc.type;
  out.descriptorCount = desc.count;
  out.stageFlags = desc.stages;
  out.pImmutableSamplers = desc.immutable_samplers;
  return out;
}

util::Status validate_desc(const DescriptorSetLayout::CreateInfo &ci) {
  QUARK_TRY_STATUS(validate(ci.device));

  QUARK_ENSURE(ci.desc.bindings.size() <= kMaxDescriptorBindings,
               QUARK_ERR(util::Errc::InvalidArg,
                         "descriptor set layout has too many bindings"));

  for (const DescriptorBindingDesc &binding : ci.desc.bindings) {
    QUARK_ENSURE(
        binding.type != VK_DESCRIPTOR_TYPE_MAX_ENUM,
        QUARK_ERR(util::Errc::InvalidArg,
                  "descriptor set layout binding has invalid descriptor type"));

    QUARK_ENSURE(
        binding.count > 0,
        QUARK_ERR(util::Errc::InvalidArg,
                  "descriptor set layout binding has zero descriptor count"));

    QUARK_ENSURE(
        binding.stages != 0,
        QUARK_ERR(util::Errc::InvalidArg,
                  "descriptor set layout binding has no shader stages"));
  }

  QUARK_OK();
}

} // namespace

util::Status DescriptorSetLayout::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate_desc(ci));

  destroy();

  device_ = ci.device;
  allocator_ = ci.allocator;

  std::array<VkDescriptorSetLayoutBinding, kMaxDescriptorBindings> bindings{};

  uint32_t binding_count = 0;
  for (const DescriptorBindingDesc &binding : ci.desc.bindings) {
    bindings[binding_count++] = to_vk_bindings(binding);
  }

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.flags = ci.flags;
  layout_info.bindingCount = binding_count;
  layout_info.pBindings = bindings.data();

  QUARK_VK_TRY(vkCreateDescriptorSetLayout(device_.device, &layout_info,
                                           allocator_, &handle_));

  QUARK_OK();
}

void DescriptorSetLayout::destroy() noexcept {
  if (device_.device != VK_NULL_HANDLE && handle_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_.device, handle_, allocator_);
  }

  handle_ = VK_NULL_HANDLE;
  device_ = {};
  allocator_ = nullptr;
}

} // namespace quark::vk
