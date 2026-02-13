#ifndef QUARK_VULKAN_CONTEXT_HPP
#define QUARK_VULKAN_CONTEXT_HPP

// NOLINTBEGIN(misc-include-cleaner)

#include <array>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

using std::array;
using std::vector;

struct GLFWwindow;

namespace quark {

class VulkanContext final {
public:
  VulkanContext();
  ~VulkanContext();

  void run();

  VulkanContext(const VulkanContext &) = delete;
  VulkanContext &operator=(const VulkanContext &) = delete;
  VulkanContext(VulkanContext &&) = delete;
  VulkanContext &operator=(VulkanContext &&) = delete;

private:
  void create_window();
  void create_instance();
  void create_debug_messenger();
  void create_surface();
  void pick_device();
  void create_logical_device();
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

  GLFWwindow *window_{nullptr};
  VkInstance instance_{VK_NULL_HANDLE};
  VkDebugUtilsMessengerEXT debug_messenger_{VK_NULL_HANDLE};
  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
  VkDevice device_{VK_NULL_HANDLE};
  VkQueue graphics_queue_{VK_NULL_HANDLE};
  VkQueue present_queue_{VK_NULL_HANDLE};
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

  uint32_t present_queue_family_index_{0};
  uint32_t graphics_queue_family_index_{0};
  bool glfw_initialised_{false};
  bool enable_debug_messenger_{false};
};

} // namespace quark

#endif // QUARK_VULKAN_CONTEXT_HPP

// NOLINTEND(misc-include-cleaner)
