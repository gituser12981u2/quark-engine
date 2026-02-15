#pragma once

#include <quark/utils/raii.hpp>
#include <quark/vk/instance/details/debug_messenger.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <quark/vk/instance/details/instance_handle.hpp>
#include <quark/vk/instance/details/instance_registry.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class InstanceBundle final {
public:
  InstanceBundle() = default;
  explicit InstanceBundle(const Instance::CreateInfo &ci);
  ~InstanceBundle() { destroy(); }

  // TODO: assert safe move semantics in instance and debug messenger
  QUARK_MOVE_ONLY(InstanceBundle);

  void create(const Instance::CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return registry_.alive(handle_); }

  /// Opaque handle
  [[nodiscard]] InstanceHandle handle() const noexcept { return handle_; }

  /// Resolve to raw VkInstance.
  [[nodiscard]] VkInstance vk_instance() const noexcept;

  [[nodiscard]] const DebugMessenger *debug_messenger() const noexcept;

private:
  InstanceRegistry registry_;
  InstanceHandle handle_{};
};

} // namespace quark::vk
