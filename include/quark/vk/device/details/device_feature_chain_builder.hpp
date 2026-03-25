#pragma once

#include <array>
#include <quark/utils/result.hpp>
#include <quark/vk/device/details/device_capabilities.hpp>

namespace quark::vk::details {

/**
 * @class Vulkan12FeatureSet
 * @brief Semantic grouping of Vulkan 1.2-era device features.
 *
 * This is an engine-side representation, no a Vulkan ABI type.
 */
struct Vulkan12FeatureSet {
  bool timeline_semaphore = false;
};

/**
 * @class Vulkan13FeatureSet
 * @brief Semantic grouping of Vulkan 1.3-era device features.
 *
 * This is an engine-side representation, no a Vulkan ABI type.
 */
struct Vulkan13FeatureSet {
  bool dynamic_rendering = false;
  bool synchronization2 = false;
};

/**
 * @class FeatureSets
 * @brief Aggregate semantic feature state used by the builder.
 */
struct FeatureSets {
  Vulkan12FeatureSet vk12{};
  Vulkan13FeatureSet vk13{};
};

/**
 * @brief Semantic bucket used by a feature descriptor.
 *
 * This lets a descriptor identify which semantic feature group owns the actual
 * bool that stores support/request state.
 */
enum class FeatureBucket : uint8_t {
  Vulkan12,
  Vulkan13,
};

/**
 * @class FeatureDescriptor
 * @brief Static description of one engine-level device feature.
 *
 * A descriptor is the canonical registry entry for a feature within the
 * builder. It ties together:
 * - the engine-visible feature bit,
 * - a stable log/debug name.
 * - the semantic bucket that owns the feature,
 * - and the member pointer used to access the semantic bool.
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
   * @brief Semantic bucket that owns the bool for this feature.
   */
  FeatureBucket bucket{};

  bool Vulkan12FeatureSet::*vk12_member = nullptr;
  bool Vulkan13FeatureSet::*vk13_member = nullptr;
};

/**
 * @class DeviceFeatureChainBuilder
 * @brief Builds and owns the Vulkan feature query/create chain for logical
 * device creation.
 *
 * This class exists to solve three problems:
 *
 * 1. Centralize Vulkan feature support queries and feature enablement policy.
 * 2. Preserve a stable engine-side semantic model of features.
 * 3. Hide platform/runtime-specific pNext chain quirks behind one API.
 *
 * From reading that, it may seem this is trivially done with
 * VkPhysicalDeviceVulkan12Features and the complementary vk13 type, but
 * MoltenVk makes this difficult. On non-Apple platforms, these versioned Vulkan
 * feature structs can serialize robustly, but MoltenVk cannot do this yet.
 * Thus, this design features typical Vulkan serialization on non-Apple
 * platforms and MoltenVk specific behavior behind an Apple flag.
 *
 * This design is intentionally reversible. If MoltenVk/runtime support becomes
 * robust enough that versioned feature structs behave normally everywhere, this
 * class can be simplified back toward a more conventional Vulkan feature-chain
 * builder without changing the higher-level engine feature API.
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
   * Adding a new feature should primarily involve:
   * - extending DeviceFeature,
   * - extending the semantic feature set,
   * - adding one descriptor entyr here,
   * - and wiring one read/write mapping at the Vulkan ABI boundary.
   */
  static constexpr std::array<FeatureDescriptor, 3> kFeatureDescriptors{{
      {
          .feature = DeviceFeature::TimelineSemaphore,
          .name = "timelineSemaphore",
          .bucket = FeatureBucket::Vulkan12,
          .vk12_member = &Vulkan12FeatureSet::timeline_semaphore,
          .vk13_member = nullptr,
      },
      {
          .feature = DeviceFeature::DynamicRendering,
          .name = "dynamicRendering",
          .bucket = FeatureBucket::Vulkan13,
          .vk12_member = nullptr,
          .vk13_member = &Vulkan13FeatureSet::dynamic_rendering,
      },
      {
          .feature = DeviceFeature::Synchronization2,
          .name = "synchronization2",
          .bucket = FeatureBucket::Vulkan13,
          .vk12_member = nullptr,
          .vk13_member = &Vulkan13FeatureSet::synchronization2,
      },
  }};

  /**
   * @brief Test whether a feature bit is present in a bitmask.
   *
   * @param flags flags Bitmask to test.
   * @param feature Feature bit to check.
   * @return True is the feature bit is set.
   */
  [[nodiscard]] static bool has_flag(DeviceFeatureFlags flags,
                                     DeviceFeature feature) noexcept {
    return (flags & static_cast<DeviceFeatureFlags>(feature)) != 0;
  }

  /**
   * @brief Read one semantic feature value from a feature set.
   *
   * @param sets Semantic feature storage to read from.
   * @param desc Descriptor identifying which semantic bool to access.
   * @return Current value of the semantic feature.
   */
  [[nodiscard]] static bool
  get_feature_(const FeatureSets &sets, const FeatureDescriptor &desc) noexcept;

  /**
   * @brief Write one semantic feature value into a feature set.
   *
   * @param sets Semantic feature storage to update.
   * @param desc Descriptor identifying which semantic bool to access.
   * @param value New semantic feature value.
   */
  static void set_feature_(FeatureSets &sets, const FeatureDescriptor &desc,
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
   * @brief Initialize the Vulkan query chain used with
   * vkGetPhysicalDeviceFeatures2.
   */
  void reset_query_chain_() noexcept;

  /**
   * @brief Initialize the Vulkan device creation chain.
   *
   * This prepares platform-specific wire structs for later population by
   * write_requested_feature_().
   */
  void reset_create_chain_() noexcept;

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

  FeatureSets supported_{};
  FeatureSets requested_{};

  VkPhysicalDeviceFeatures2 features2_{};

#if defined(__APPLE__)
  VkPhysicalDeviceTimelineSemaphoreFeatures timeline_{};
  VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_{};
  VkPhysicalDeviceSynchronization2Features synchronization2_{};
#else
  VkPhysicalDeviceVulkan12Features vk12{};
  VkPhysicalDeviceVulkan13Features vk13{};
#endif
};

} // namespace quark::vk::details
