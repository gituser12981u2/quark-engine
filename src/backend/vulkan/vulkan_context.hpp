#pragma once

#include "quark/vk/frame/details/frame_cmd.hpp"
#include "quark/vk/frame/details/frame_sync.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <quark/platform/window/IWindow.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/device_bundle.hpp>
#include <quark/vk/instance/instance_bundle.hpp>
#include <vector>
#include <vulkan/vulkan.h>

using std::array;
using std::vector;

namespace quark::vk {

class VulkanContext final {
public:
  VulkanContext() = default;
  ~VulkanContext();

  util::Status run();

  VulkanContext(const VulkanContext &) = delete;
  VulkanContext &operator=(const VulkanContext &) = delete;
  VulkanContext(VulkanContext &&) = delete;
  VulkanContext &operator=(VulkanContext &&) = delete;

private:
  util::Status init();

  void create_window();

  util::Status create_instance();
  void create_surface();
  util::Status create_device();
  void create_swapchain();
  void create_swapchain_image_views();

  util::Status create_frame_cmd();
  util::Status create_frame_sync();
  util::Status record_command_buffers();

  void create_render_pass();
  void create_framebuffers();

  util::Status draw_frame();

  void cleanup_swapchain();
  util::Status recreate_swapchain();

  std::unique_ptr<platform::IWindow> window_;
  InstanceBundle instance_;
  DeviceBundle device_;

  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
  VkFormat swapchain_image_format_{VK_FORMAT_UNDEFINED};
  VkExtent2D swapchain_extent_{};
  vector<VkImage> swapchain_images_;
  vector<VkImageView> swapchain_image_views_;

  static constexpr uint32_t kMaxFramesInFlight{2};

  details::FrameCmd frame_cmd_;
  details::FrameSync frame_sync_;

  VkRenderPass render_pass_{VK_NULL_HANDLE};
  vector<VkFramebuffer> framebuffers_;

  vector<VkFence> images_in_flight_;
  uint32_t current_frame_{0};
};

} // namespace quark::vk
