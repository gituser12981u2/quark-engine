#pragma once

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
  void create_render_pass();
  void create_framebuffers();
  void create_command_pool();
  void create_command_buffers();
  void create_sync_objects();
  void draw_frame();
  void cleanup_swapchain();
  void recreate_swapchain();

  std::unique_ptr<platform::IWindow> window_;
  InstanceBundle instance_;
  DeviceBundle device_;

  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
  VkFormat swapchain_image_format_{VK_FORMAT_UNDEFINED};
  VkExtent2D swapchain_extent_{};
  vector<VkImage> swapchain_images_;
  vector<VkImageView> swapchain_image_views_;

  VkRenderPass render_pass_{VK_NULL_HANDLE};

  vector<VkFramebuffer> framebuffers_;
  VkCommandPool command_pool_{VK_NULL_HANDLE};
  vector<VkCommandBuffer> command_buffers_;

  vector<VkSemaphore> render_finished_semaphores_per_image_;

  static constexpr uint32_t kMaxFramesInFlight{2};
  array<VkSemaphore, kMaxFramesInFlight> image_available_semaphores_{};
  array<VkFence, kMaxFramesInFlight> in_flight_fences_{};
  vector<VkFence> images_in_flight_;
  uint32_t current_frame_{0};
};

} // namespace quark::vk
