#include <quark/utils/diagnostic.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/details/device_capabilities.hpp>
#include <quark/vk/device/details/device_feature_chain_builder.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

void DeviceFeatureChainBuilder::set_flag_(DeviceFeatureFlags &flags,
                                          DeviceFeature feature,
                                          bool value) noexcept {
  const auto bit = static_cast<DeviceFeatureFlags>(feature);
  if (value) {
    flags |= bit;
  } else {
    flags &= ~bit;
  }
}

void DeviceFeatureChainBuilder::reset_chain_() noexcept {
  features2_ = {};
  features2_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2_.pNext = nullptr;

  vk12_ = {};
  vk13_ = {};

  vk12_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  vk13_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

  features2_.pNext = &vk12_;
  vk12_.pNext = &vk13_;
  vk13_.pNext = nullptr;
}

[[nodiscard]] bool DeviceFeatureChainBuilder::read_supported_feature_(
    const FeatureDescriptor &desc) const noexcept {
  switch (desc.wire_struct) {
  case WireFeatureStruct::Vulkan12:
    return vk12_.*(desc.vk12_member) == VK_TRUE;

  case WireFeatureStruct::Vulkan13:
    return vk13_.*(desc.vk13_member) == VK_TRUE;
  }

  return false;
}

void DeviceFeatureChainBuilder::write_requested_feature_(
    const FeatureDescriptor &desc, bool value) noexcept {
  const VkBool32 vk_value = value ? VK_TRUE : VK_FALSE;

  switch (desc.wire_struct) {
  case WireFeatureStruct::Vulkan12:
    vk12_.*(desc.vk12_member) = vk_value;
    return;

  case WireFeatureStruct::Vulkan13:
    vk13_.*(desc.vk13_member) = vk_value;
    return;
  }
}

util::Status
DeviceFeatureChainBuilder::query_supported(VkPhysicalDevice physical_device,
                                           DeviceCapabilities &capabilities) {
  QUARK_ENSURE(physical_device != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "physical_device is null"));

  reset_chain_();
  vkGetPhysicalDeviceFeatures2(physical_device, &features2_);

  supported_features_ = 0;
  capabilities.supported_features = 0;

  for (const auto &desc : kFeatureDescriptors) {
    const bool supported = read_supported_feature_(desc);
    set_flag_(supported_features_, desc.feature, supported);

    if (supported) {
      capabilities.supported_features |=
          static_cast<DeviceFeatureFlags>(desc.feature);
    }
  }

  QUARK_OK();
}

util::Status DeviceFeatureChainBuilder::maybe_enable_(
    DeviceFeature feature, const char *name, bool supported, bool required,
    bool preferred, bool &requested_out, DeviceCapabilities &capabilities) {
  requested_out = false;

  if (!required && !preferred) {
    QUARK_OK();
  }
  QUARK_LOG_INFO("device feature requested: {} required={} supported={}", name,
                 required ? "true" : "false", supported ? "true" : "false");

  if (!supported) {
    QUARK_ENSURE(!required,
                 QUARK_ERR(util::Errc::Unsupported,
                           "Required device feature unsupported: {}", name));
    QUARK_LOG_INFO("device feature unavailable: {}", name);
    QUARK_OK();
  }

  requested_out = true;
  capabilities.enabled_features |= static_cast<DeviceFeatureFlags>(feature);
  QUARK_LOG_INFO("device feature enabled: {}", name);
  QUARK_OK();
}

util::Status
DeviceFeatureChainBuilder::select_requested(DeviceFeatureFlags required,
                                            DeviceFeatureFlags preferred,
                                            DeviceCapabilities &capabilities) {
  requested_features_ = 0;
  capabilities.enabled_features = 0;

  for (const auto &desc : kFeatureDescriptors) {
    const bool is_required = has_flag_(required, desc.feature);
    const bool is_preferred = has_flag_(preferred, desc.feature);
    const bool is_supported = has_flag_(supported_features_, desc.feature);

    bool request_this = false;
    QUARK_TRY_STATUS(maybe_enable_(desc.feature, desc.name, is_supported,
                                   is_required, is_preferred, request_this,
                                   capabilities));

    if (request_this) {
      set_flag_(requested_features_, desc.feature, true);
    }
  }

  reset_chain_();

  for (const auto &desc : kFeatureDescriptors) {
    write_requested_feature_(desc,
                             has_flag_(requested_features_, desc.feature));
  }

  QUARK_OK();
}

} // namespace quark::vk::details
