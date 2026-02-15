#pragma once

#include <cstdint>
#include <quark/platform/window/IWindow.hpp>
#include <quark/utils/raii.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class Swapchain final {
public:
  struct CreateInfo {
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    uint32_t graphics_queue_family_index = 0;
    uint32_t present_queue_family_index = 0;

    const platform::IWindow *window = nullptr;

    VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;

    VkPresentModeKHR preferred_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    VkFormat preferred_format = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR preferred_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  };

  Swapchain() = default;
  ~Swapchain() { reset(); }

  QUARK_MOVE_ONLY(Swapchain);

  void create(const CreateInfo &ci);
  void reset() noexcept;

  [[nodiscard]] VkSwapchainKHR handle() const noexcept { return swapchain_; }
  [[nodiscard]] VkFormat format() const noexcept { return image_format_; }
  [[nodiscard]] VkExtent2D extent() const noexcept { return extent_; }

  [[nodiscard]] const std::vector<VkImage> &images() const noexcept {
    return images_;
  }
  [[nodiscard]] const std::vector<VkImageView> &image_views() const noexcept {
    return image_views_;
  }

private:
  VkDevice device_ = VK_NULL_HANDLE; // non-owning

  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkFormat image_format_ = VK_FORMAT_UNDEFINED;
  VkExtent2D extent_{};

  std::vector<VkImage> images_;
  std::vector<VkImageView> image_views_;
};

} // namespace quark::vk
