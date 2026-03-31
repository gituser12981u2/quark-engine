#pragma once

#include <array>
#include <quark/utils/result.hpp>
#include <quark/vk/device/details/device_capabilities.hpp>

namespace quark::vk::details {

/**
 * @enum WireFeatureStruct
 * @brief Identifies which Vulkan feature struct own a feature field.
 *
 * DeviceFeatureChainBuilder stores features in a descriptor table and uses
 * this to determine which Vulkan wire format struct should be read from or
 * written to for a given feature.
 */
enum class WireFeatureStruct : uint8_t {
  Vulkan12,
  Vulkan13,
};

/**
 * @class FeatureDescriptor
 * @brief Static description of one engine-level device feature.
 *
 * A descriptor is the canonical registry entry for a feature within the
 * builder. It ties together:
 * - the engine-visible feature bit
 * - a stable log/debug name
 * - the Vulkan wire format field
 */
struct FeatureDescriptor {
  /**
   * @brief Engine-level feature bit represented by this descriptor.
   */
  DeviceFeature feature{};

  /**
   * @brief Human-readable feature name used in logs/errors.
   */
  const char *name = nullptr;

  /**
   * @brief Identifies which Vulkan feature struct owns the mapped field.
   */
  WireFeatureStruct wire_struct{};

  VkBool32 VkPhysicalDeviceVulkan12Features::*vk12_member = nullptr;
  VkBool32 VkPhysicalDeviceVulkan13Features::*vk13_member = nullptr;

  /**
   * @brief Creates a descriptor for a Vulkan 1.2 feature field.
   *
   * @param feature Engine level feature bit.
   * @param name Human-readable feature name.
   * @param member Member pointer into VkPhysicalDeviceVulkan12Features.
   * @return Fully initialized descriptor.
   */
  [[nodiscard]] static constexpr FeatureDescriptor
  make_vk12(DeviceFeature feature, const char *name,
            VkBool32 VkPhysicalDeviceVulkan12Features::*member) noexcept {
    return FeatureDescriptor{
        .feature = feature,
        .name = name,
        .wire_struct = WireFeatureStruct::Vulkan12,
        .vk12_member = member,
        .vk13_member = nullptr,
    };
  }

  /**
   * @brief Create a descriptor for a Vulkan 1.3 feature field.
   *
   * @param feature Engine level feature bit.
   * @param name Human-readable feature name.
   * @param member Member pointer into VkPhysicalDeviceVulkan13Features.
   * @return Fully initialized descriptor.
   */
  [[nodiscard]] static constexpr FeatureDescriptor
  make_vk13(DeviceFeature feature, const char *name,
            VkBool32 VkPhysicalDeviceVulkan13Features::*member) noexcept {
    return FeatureDescriptor{
        .feature = feature,
        .name = name,
        .wire_struct = WireFeatureStruct::Vulkan13,
        .vk12_member = nullptr,
        .vk13_member = member,
    };
  }
};

/**
 * @class DeviceFeatureChainBuilder
 * @brief Builds and owns the Vulkan feature query/create chain for logical
 * device creation.
 *
 * This class centralizes Vulkan feature enabling and querying. This makes it
 * simple for developers to add new necessary or optional features.
 *
 * The builder uses a descriptor table as the single registry of engine-visible
 * features. This keeps feature support checks, enablement policy, and Vulkan
 * field mapping consistent and reduces the number of places that must be edited
 * when adding a new feature.
 */
class DeviceFeatureChainBuilder final {
public:
  DeviceFeatureChainBuilder() = default;

  util::Status query_supported(VkPhysicalDevice physical_device,
                               DeviceCapabilities &capabilities);

  util::Status select_requested(DeviceFeatureFlags required,
                                DeviceFeatureFlags preferred,
                                DeviceCapabilities &capabilities);

  /**
   * @brief Return the pNext chain root for VkDeviceCreateInfo.
   *
   * The returned pointer refers to storage owned by this builder and remains
   * valid only while the builder object is alive.
   *
   * @return Pointer to the VkPhysicalDeviceFeatures2 root used for device
   * creation.
   */
  [[nodiscard]] const void *create_pnext() const noexcept {
    return &features2_;
  }

private:
  /**
   * @brief Canonical registry of all device features known to this builder.
   *
   * Adding a new feature involves:
   * - adding one descriptor entry
   */
  static constexpr std::array<FeatureDescriptor, 3> kFeatureDescriptors{
      {FeatureDescriptor::make_vk12(
           DeviceFeature::TimelineSemaphore, "timelineSemaphore",
           &VkPhysicalDeviceVulkan12Features::timelineSemaphore),
       FeatureDescriptor::make_vk13(
           DeviceFeature::DynamicRendering, "dynamicRendering",
           &VkPhysicalDeviceVulkan13Features::dynamicRendering),
       FeatureDescriptor::make_vk13(
           DeviceFeature::Synchronization2, "synchronization2",
           &VkPhysicalDeviceVulkan13Features::synchronization2)}};

  /**
   * @brief Test whether a feature bit is present in a bitmask.
   *
   * @param flags flags Bitmask to test.
   * @param feature Feature bit to check.
   * @return True is the feature bit is set.
   */
  [[nodiscard]] static bool has_flag_(DeviceFeatureFlags flags,
                                      DeviceFeature feature) noexcept {
    return (flags & static_cast<DeviceFeatureFlags>(feature)) != 0;
  }

  /**
   * @brief Set or clear a feature bit in a bitmask.
   *
   * @param flags Bitmask to update.
   * @param feature Feature bit to modify.
   * @param value True to set the bit, false to clear it.
   */
  static void set_flag_(DeviceFeatureFlags &flags, DeviceFeature feature,
                        bool value) noexcept;

  /**
   * @brief Read one supported feature value from the Vulkan wire chain.
   *
   * @param desc Descriptor identifying which semantic bool to access.
   * @return True if the feature is reported supported by the quired wire chain.
   */
  [[nodiscard]] bool
  read_supported_feature_(const FeatureDescriptor &desc) const noexcept;

  /**
   * @brief Write one requested feature value into the Vulkan create chain.
   *
   * @param desc Descriptor identifying which semantic bool to access.
   * @param value Requested semantic value to serialize into the chain.
   */
  void write_requested_feature_(const FeatureDescriptor &desc,
                                bool value) noexcept;

  /**
   * @brief Initialize the Vulkan feature chain structure.
   *
   * Prepares the VkPhysicalDeviceFeatures2 root and attaches the versioned
   * Vulkan feature structs int he correct pNext order.
   */
  void reset_chain_() noexcept;

  /**
   * @brief Apply required/preferred feature policy to one feature.
   *
   * @param feature Engine-level feature bit.
   * @param name Human-readable feature name for logs/errors.
   * @param supported Whether the feature is supported by the queried device.
   * @param required Whether the feature is required by the engine policy.
   * @param preferred Whether the feature is preferred by the engine policy.
   * @param requested_out Ouput set to true when the feature should be enabled.
   * @param capabilities Capability record updated with enabled feature bits.
   * @return Success of an unsupported-feature error when a required feature is
   * unavailable.
   */
  util::Status static maybe_enable_(DeviceFeature feature, const char *name,
                                    bool supported, bool required,
                                    bool preferred, bool &requested_out,
                                    DeviceCapabilities &capabilities);

  /**
   * @brief Bitset of features reported supported by the physical device.
   */
  DeviceFeatureFlags supported_features_ = 0;

  /**
   * @brief Bitset of features selected for logical-device creation.
   */
  DeviceFeatureFlags requested_features_ = 0;

  /**
   * @brief Root of the Vulkan feature query/create chain.
   */
  VkPhysicalDeviceFeatures2 features2_{};

  VkPhysicalDeviceVulkan12Features vk12_{};
  VkPhysicalDeviceVulkan13Features vk13_{};
};

} // namespace quark::vk::details
