#pragma once

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/presentation/details/swapchain.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class PresenterBundle final {
public:
  struct CreateInfo {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    uint32_t graphics_queue_family_index = 0;
    uint32_t present_queue_family_index = 0;

    const platform::IWindow *window = nullptr;

    VkPresentModeKHR preferred_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    VkFormat preferred_format = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR preferred_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  };

  PresenterBundle() = default;
  ~PresenterBundle() { destroy(); }

  QUARK_MOVE_ONLY(PresenterBundle);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  [[nodiscard]] util::Status recreate_swapchain();
  void destroy_swapchain() noexcept;
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return surface_ != VK_NULL_HANDLE && swapchain_.handle() != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkSurfaceKHR surface() const noexcept { return surface_; }
  [[nodiscard]] const Swapchain &swapchain() const noexcept {
    return swapchain_;
  }
  [[nodiscard]] Swapchain &swapchain() noexcept { return swapchain_; }

private:
  CreateInfo create_info_{};
  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  Swapchain swapchain_;
};

} // namespace quark::vk