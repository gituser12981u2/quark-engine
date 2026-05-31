#pragma once

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

namespace details {

class DebugMessenger;

} // namespace details

/**
 * @brief Owns a Vulkan instance (VkInstance).
 *
 * This is the public facade used by engine code outside the instance subsystem.
 * Internally it stores an opaque handle into details::InstanceRegistry.
 *
 * Lifetime:
 * - create() acquires a live instance slot in the registry
 * - destroy() release it (idempotent).
 *
 * Threading:
 * - Not thread-safe; external synchronization required.
 */
class InstanceBundle final {
public:
  struct CreateInfo {
    /// User facing instance settings.
    details::Instance::CreateInfo instance{};
  };

  InstanceBundle() = default;
  ~InstanceBundle() { destroy(); }

  // TODO: assert safe move semantics in instance and debug messenger
  QUARK_MOVE_ONLY(InstanceBundle);

  /// Acquires a live instance slot in the registry
  [[nodiscard]] util::Status create(const CreateInfo &ci);

  /// Releases the live instance slot in the registry (idempotent).
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return instance_.valid(); }

  /**
   * Resolves to raw VkInstance.
   *
   * Equivalent to view().instance.
   */
  [[nodiscard]] VkInstance vk_instance() const noexcept;

  [[nodiscard]] const details::DebugMessenger *debug_messenger() const noexcept;

private:
  details::Instance instance_;
};

} // namespace quark::vk
