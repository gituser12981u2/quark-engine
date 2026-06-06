#pragma once

#include "quark/platform/shader/details/shader_registry.hpp"
#include "quark/vk/pipeline/graphics_pipeline_bundle.hpp"

#include <array>
#include <cstdint>

#include <quark/engine/retire/retirement_queue.hpp>

#include <quark/utils/result.hpp>
#include <quark/vk/device/device_bundle.hpp>
#include <quark/vk/frame/frame_bundle.hpp>
#include <quark/vk/instance/instance_bundle.hpp>

#include <quark/vk/sync/gpu_timeline.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#if !QUARK_HEADLESS
#include <memory>
#include <quark/platform/window/IWindow.hpp>
#include <quark/vk/presentation/presenter_bundle.hpp>
#endif

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
  enum class RenderPath : uint8_t {
    Vulkan12Fallback,
    Vulkan13DynamicRendering,
  };

  util::Status init();

#if !QUARK_HEADLESS
  void create_window();
#endif

  util::Status create_instance();
  util::Status create_device();
  util::Status create_gpu_timeline();
  void resolve_render_path();

#if !QUARK_HEADLESS
  util::Status create_presenter();
#endif

  util::Status create_frame();
  util::Status create_retirement_queue();

#if !QUARK_HEADLESS
  util::Status record_command_buffer(uint32_t image_index);

  util::Status create_render_pass();
  util::Status create_framebuffers();
  util::Status submit_frame(VkCommandBuffer command_buffer,
                            VkSemaphore image_available,
                            VkSemaphore render_finished, uint64_t signal_value);

  util::Status draw_frame();

  void cleanup_swapchain();
  util::Status recreate_swapchain();
#endif

  InstanceBundle instance_;
  DeviceBundle device_;

#if !QUARK_HEADLESS
  std::unique_ptr<platform::IWindow> window_;
  PresenterBundle presenter_;
#endif

  static constexpr uint32_t kMaxFramesInFlight{2};

  FrameBundle frame_;
  GpuTimeline gpu_timeline_; // global timeline semaphore
  RetirementQueue retirement_queue_;

  RenderPath render_path_{RenderPath::Vulkan12Fallback};
  PFN_vkQueueSubmit2 queue_submit2_{nullptr};
  PFN_vkCmdBeginRendering cmd_begin_rendering_{nullptr};
  PFN_vkCmdEndRendering cmd_end_rendering_{nullptr};
  PFN_vkCmdPipelineBarrier2 cmd_pipeline_barrier2_{nullptr};

#if !QUARK_HEADLESS
  VkRenderPass render_pass_{VK_NULL_HANDLE};
  vector<VkFramebuffer> framebuffers_;

  vector<uint64_t> images_in_flight_;
  vector<bool> swapchain_images_initialized_;
  uint32_t current_frame_{};
#endif

  GraphicsPipelineBundle triangle_pipeline_;
  ShaderRegistry shader_registry_;

  // Triangle rendering resources
  VkBuffer triangle_vertex_buffer_{VK_NULL_HANDLE};
  VkDeviceMemory triangle_vertex_memory_{VK_NULL_HANDLE};
  uint32_t triangle_vertex_count_{};

  // Triangle setup/cleanup
  util::Status create_triangle_pipeline();
  void destroy_triangle_pipeline();
  util::Status create_triangle_vertex_buffer();
  void destroy_triangle_vertex_buffer();
};

} // namespace quark::vk
