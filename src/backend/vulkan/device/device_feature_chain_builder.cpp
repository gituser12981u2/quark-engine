#include <quark/utils/diagnostic.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/details/device_capabilities.hpp>
#include <quark/vk/device/details/device_feature_chain_builder.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

[[nodiscard]] bool DeviceFeatureChainBuilder::get_feature_(
    const FeatureSets &sets, const FeatureDescriptor &desc) noexcept {
  switch (desc.bucket) {
  case FeatureBucket::Vulkan12:
    return sets.vk12.*(desc.vk12_member);
  case FeatureBucket::Vulkan13:
    return sets.vk13.*(desc.vk13_member);
  }

  return false;
}

void DeviceFeatureChainBuilder::set_feature_(FeatureSets &sets,
                                             const FeatureDescriptor &desc,
                                             bool value) noexcept {
  switch (desc.bucket) {
  case FeatureBucket::Vulkan12:
    sets.vk12.*(desc.vk12_member) = value;
    return;
  case FeatureBucket::Vulkan13:
    sets.vk13.*(desc.vk13_member) = value;
    return;
  }
}

void DeviceFeatureChainBuilder::reset_query_chain_() noexcept {
  // On Apple avoid relying on the Vulkan 1.3 aggregate feature struct path,
  // because MoltenVk has some issues around VkPhysicalDeviceVulkan13Features.
  supported_ = {};
  requested_ = {};

  features2_ = {};
  features2_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2_.pNext = nullptr;

#if defined(__APPLE__)
  timeline_ = {};
  dynamic_rendering_ = {};
  synchronization2_ = {};

  timeline_.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
  dynamic_rendering_.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  synchronization2_.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;

  features2_.pNext = &timeline_;
  timeline_.pNext = &dynamic_rendering_;
  dynamic_rendering_.pNext = &synchronization2_;
  synchronization2_.pNext = nullptr;
#else
  vk12_ = {};
  vk13_ = {};

  vk12_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  vk13_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

  features2_.pNext = &vk12_;
  vk12_.pNext = &vk13_;
  vk13_.pNext = nullptr;
#endif
}

void DeviceFeatureChainBuilder::reset_create_chain_() noexcept {
  features2_ = {};
  features2_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2_.pNext = nullptr;

#if defined(__APPLE__)
  timeline_ = {};
  dynamic_rendering_ = {};
  synchronization2_ = {};

  timeline_.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
  dynamic_rendering_.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  synchronization2_.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;

  timeline_.timelineSemaphore =
      requested_.vk12.timeline_semaphore ? VK_TRUE : VK_FALSE;
  dynamic_rendering_.dynamicRendering =
      requested_.vk13.dynamic_rendering ? VK_TRUE : VK_FALSE;
  synchronization2_.synchronization2 =
      requested_.vk13.synchronization2 ? VK_TRUE : VK_FALSE;

  features2_.pNext = &timeline_;
  timeline_.pNext = &dynamic_rendering_;
  dynamic_rendering_.pNext = &synchronization2_;
  synchronization2_.pNext = nullptr;
#else
  vk12_ = {};
  vk13_ = {};

  vk12_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  vk13_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

  features2_.pNext = &vk12_;
  vk12_.pNext = &vk13_;
  vk13_.pNext = nullptr;
#endif
}

[[nodiscard]] bool DeviceFeatureChainBuilder::read_supported_feature_(
    const FeatureDescriptor &desc) const noexcept {
  switch (desc.feature) {
  case DeviceFeature::TimelineSemaphore:
#if defined(__APPLE__)
    return timeline_.timelineSemaphore == VK_TRUE;
#else
    return vk12_.timelineSemaphore == VK_TRUE;
#endif

  case DeviceFeature::DynamicRendering:
#if defined(__APPLE__)
    return dynamic_rendering_.dynamicRendering == VK_TRUE;
#else
    return vk13_.dynamicRendering == VK_TRUE;
#endif

  case DeviceFeature::Synchronization2:
#if defined(__APPLE__)
    return synchronization2_.synchronization2 == VK_TRUE;
#else
    return vk13_.synchronization2 == VK_TRUE;
#endif
  }

  return false;
}

void DeviceFeatureChainBuilder::write_requested_feature_(
    const FeatureDescriptor &desc, bool value) noexcept {
  const VkBool32 vk_value = value ? VK_TRUE : VK_FALSE;

  switch (desc.feature) {
  case DeviceFeature::TimelineSemaphore:
#if defined(__APPLE__)
    timeline_.timelineSemaphore = vk_value;
#else
    vk12_.timelineSemaphore = vk_value;
#endif
    return;

  case DeviceFeature::DynamicRendering:
#if defined(__APPLE__)
    dynamic_rendering_.dynamicRendering = vk_value;
#else
    vk13_.dynamicRendering = vk_value;
#endif
    return;

  case DeviceFeature::Synchronization2:
#if defined(__APPLE__)
    synchronization2_.synchronization2 = vk_value;
#else
    vk13_.synchronization2 = vk_value;
#endif
    return;
  }
}

util::Status
DeviceFeatureChainBuilder::query_supported(VkPhysicalDevice physical_device,
                                           DeviceCapabilities &capabilities) {
  QUARK_ENSURE(physical_device != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "physical_device is null"));

  reset_query_chain_();
  vkGetPhysicalDeviceFeatures2(physical_device, &features2_);

  supported_ = {};
  capabilities.supported_features = 0;

  for (const auto &desc : kFeatureDescriptors) {
    set_feature_(supported_, desc, read_supported_feature_(desc));
    if (get_feature_(supported_, desc)) {
      capabilities.supported_features |=
          static_cast<DeviceFeatureFlags>(desc.feature);
    }
  }

  QUARK_OK();
}

util::Status DeviceFeatureChainBuilder::maybe_enable_(
    DeviceFeature feature, const char *name, bool supported, bool required,
    bool preferred, bool &requested_out, DeviceCapabilities &capabilities) {
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
  requested_ = {};
  capabilities.enabled_features = 0;

  for (const auto &desc : kFeatureDescriptors) {
    const bool is_required = has_flag(required, desc.feature);
    const bool is_preferred = has_flag(preferred, desc.feature);
    const bool is_supported = get_feature_(supported_, desc);

    bool request_this = false;
    QUARK_TRY_STATUS(maybe_enable_(desc.feature, desc.name, is_supported,
                                   is_required, is_preferred, request_this,
                                   capabilities));

    if (request_this) {
      set_feature_(requested_, desc, true);
    }
  }

  reset_create_chain_();

  for (const auto &desc : kFeatureDescriptors) {
    write_requested_feature_(desc, get_feature_(requested_, desc));
  }

  QUARK_OK();
}

} // namespace quark::vk::details
