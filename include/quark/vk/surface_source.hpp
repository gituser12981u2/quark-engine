#pragma once

#include <quark/utils/raii.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

/**
 * @brief Window-provided capability for Vulkan presentation integration.
 *
 * Implementations are owned by a platform window and exposed via
 * platform::IWindow::query_interface() using
 * platform::interface_id<IVulkanSurfaceSource>().
 */
struct IVulkanSurfaceSource {
  IVulkanSurfaceSource() = default;
  virtual ~IVulkanSurfaceSource() = default;

  QUARK_NO_COPY_NO_MOVE(IVulkanSurfaceSource);

  /**
   * @brief Returns the Vulkan instance extensions required by this window
   * system.
   *
   * @return A list of extension name strings (null-terminated C strings).
   */
  [[nodiscard]] virtual std::vector<const char *>
  required_instance_extensions() const = 0;

  /**
   * @brief Creates a VkSurfaceKHR for the given VkInstance.
   *
   * @param instance Vulkan instance the surface will be created against.
   *
   * @return A valid VkSurfaceKHR on success, or VK_NULL_HANDLE on failure.
   *
   * @note Ownership of the returned surface is transferred to the caller.
   * The caller must destroy it with vkDestroySurfaceKHR().
   */
  [[nodiscard]] virtual VkSurfaceKHR
  create_surface(VkInstance instance) const = 0;
};

} // namespace quark::vk
