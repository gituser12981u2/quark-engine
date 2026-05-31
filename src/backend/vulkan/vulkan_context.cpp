#include "vulkan_context.hpp"

// TODO: move test to testing system when possible
#include "quark/engine/retire/retirement_queue.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <new>

#if !QUARK_HEADLESS
#include <memory>
#include <quark/platform/window/glfw_window.hpp>
#include <quark/platform/window/interface_query.hpp>
#endif

#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/instance/instance_bundle.hpp>
#include <quark/vk/surface_source.hpp>
#include <quark/vk/sync/gpu_timeline.hpp>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

using std::array;
using std::vector;

// TODO: move test to testing system when possible
namespace {

struct RetireTestPayload {
  uint64_t id = 0;
  uint64_t retire_at = 0;
};

void retire_test_run(void *ctx) noexcept {
  auto *payload = static_cast<RetireTestPayload *>(ctx);
  if (payload == nullptr) {
    return;
  }

  QUARK_LOG_INFO_MOD("retire.test", "drained test payload id={} retire_at={}",
                     payload->id, payload->retire_at);
}

void retire_test_cleanup(void *ctx) noexcept {
  auto *payload = static_cast<RetireTestPayload *>(ctx);
  delete payload;
}

[[maybe_unused]] util::Status
enqueue_retire_test(quark::vk::RetirementQueue &queue, uint64_t retire_at,
                    uint64_t id) {
  auto *payload = new (std::nothrow) RetireTestPayload{
      .id = id,
      .retire_at = retire_at,
  };

  QUARK_ENSURE(payload != nullptr,
               QUARK_ERR(util::Errc::OutOfMemory,
                         "failed to allocate RetireTestPayload"));

  quark::vk::RetirementQueue::Task task{};
  task.fn = &retire_test_run;
  task.cleanup = &retire_test_cleanup;
  task.ctx = payload;

  QUARK_LOG_INFO_MOD("retire.test", "enqueue test payload id={} retire_at={}",
                     id, retire_at);

  auto res = queue.enqueue(retire_at, task);
  if (!res) {
    retire_test_cleanup(payload);
    return util::unexpected(std::move(res.error()));
  }

  QUARK_OK();
}

} // namespace

namespace {

#if !QUARK_HEADLESS
constexpr int kWindowWidth{1280};
constexpr int kWindowHeight{720};
constexpr std::string_view kWindowTitle = "quark-engine";

constexpr const char *kSwapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
#endif

constexpr bool kEnableValidationLayers =
#ifndef NDEBUG
    true;
#else
    false;
#endif

constexpr array<const char *, 1> kValidationLayers{
    "VK_LAYER_KHRONOS_validation",
};

auto api_version_at_least(uint32_t version, uint32_t major, uint32_t minor)
    -> bool {
  if (VK_VERSION_MAJOR(version) != major) {
    return VK_VERSION_MAJOR(version) > major;
  }

  return VK_VERSION_MINOR(version) >= minor;
}

auto choose_instance_api_version() -> uint32_t {
  uint32_t loader_version = VK_API_VERSION_1_0;
#if defined(VK_VERSION_1_1)
  vkEnumerateInstanceVersion(&loader_version);
#endif

  if (api_version_at_least(loader_version, 1, 3)) {
    return VK_API_VERSION_1_3;
  }

  if (api_version_at_least(loader_version, 1, 2)) {
    return VK_API_VERSION_1_2;
  }

  return VK_API_VERSION_1_0;
}

auto check_validation_layer_support() -> bool {
  uint32_t layer_count{0};
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char *const layer_name : kValidationLayers) {
    const auto found = std::ranges::any_of(
        available_layers, [layer_name](const VkLayerProperties &properties) {
          return std::strcmp(properties.layerName, layer_name) == 0;
        });

    if (!found) {
      return false;
    }
  }

  return true;
}

#if !QUARK_HEADLESS

auto create_probe_surface(VkInstance instance,
                          const quark::platform::IWindow &window)
    -> util::Result<VkSurfaceKHR> {
  QUARK_ENSURE(
      instance != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidArg, "probe surface instance is null"));

  const auto *source =
      quark::platform::query<quark::vk::IVulkanSurfaceSource>(window);
  QUARK_ENSURE(source != nullptr,
               QUARK_ERR(util::Errc::Unsupported,
                         "Window does not provide IVulkanSurfaceSource"));

  VkSurfaceKHR surface = source->create_surface(instance);
  QUARK_ENSURE(surface != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::ApiError,
                         "probe surface creation returned VK_NULL_HANDLE"));

  return surface;
}

void destroy_probe_surface(VkInstance instance,
                           VkSurfaceKHR &surface) noexcept {
  if (instance == VK_NULL_HANDLE || surface == VK_NULL_HANDLE) {
    return;
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  surface = VK_NULL_HANDLE;
}

#endif

} // namespace

namespace quark::vk {

util::Status VulkanContext::init() {
#if !QUARK_HEADLESS
  create_window();
#endif

  QUARK_TRY_STATUS(create_instance());

  QUARK_TRY_STATUS(create_device());
  QUARK_TRY_STATUS(create_gpu_timeline());
  QUARK_TRY_STATUS(create_retirement_queue());

#if !QUARK_HEADLESS
  QUARK_TRY_STATUS(create_presenter());

  images_in_flight_.assign(presenter_.swapchain().images().size(), 0);
  swapchain_images_initialized_.assign(presenter_.swapchain().images().size(),
                                       false);

  QUARK_TRY_STATUS(create_frame());

  QUARK_TRY_STATUS(create_render_pass());
  QUARK_TRY_STATUS(create_framebuffers());
#endif

  QUARK_OK();
}

VulkanContext::~VulkanContext() {
  if (device_.validate()) {
    VkDevice device = device_.vk_device();
    vkDeviceWaitIdle(device);
  }

  frame_.destroy();
  retirement_queue_.destroy();
  gpu_timeline_.destroy();

#if !QUARK_HEADLESS
  cleanup_swapchain();
  presenter_.destroy();
#endif

  device_.destroy();
  instance_.destroy();
}

util::Status VulkanContext::run() {
  QUARK_TRY_STATUS(init());

#if QUARK_HEADLESS
  QUARK_LOG_INFO("Vulkan initialised successfully in headless mode.");

  if (device_.validate()) {
    vkDeviceWaitIdle(device_.vk_device());
  }

  QUARK_OK();
#else
  QUARK_LOG_INFO("Vulkan initialised successfully.");

  while (!window_->should_close()) {
    window_->poll_events();
    QUARK_TRY_STATUS(draw_frame());
  }

  if (device_.validate()) {
    vkDeviceWaitIdle(device_.vk_device());
  }

  QUARK_OK();
#endif
}

util::Status VulkanContext::create_device() {

  DeviceBundle::CreateInfo ci{};
  ci.device.instance = instance_.vk_instance();
  ci.device.required_features = static_cast<details::DeviceFeatureFlags>(
      details::DeviceFeature::TimelineSemaphore);
  ci.device.preferred_features = static_cast<details::DeviceFeatureFlags>(
                                     details::DeviceFeature::DynamicRendering) |
                                 static_cast<details::DeviceFeatureFlags>(
                                     details::DeviceFeature::Synchronization2);

#if !QUARK_HEADLESS
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  QUARK_TRY_ASSIGN(surface,
                   create_probe_surface(instance_.vk_instance(), *window_));

  ci.device.surface = surface;
  ci.device.required_extensions = {kSwapchainExtension};
#else
  ci.device.surface = VK_NULL_HANDLE;
  ci.device.required_extensions = {};
#endif

  auto device_status = device_.create(ci);

#if !QUARK_HEADLESS
  destroy_probe_surface(instance_.vk_instance(), surface);
#endif

  if (!device_status) {
    return util::unexpected(std::move(device_status.error()));
  }

  queue_submit2_ = std::bit_cast<PFN_vkQueueSubmit2>(
      vkGetDeviceProcAddr(device_.vk_device(), "vkQueueSubmit2"));
  cmd_begin_rendering_ = std::bit_cast<PFN_vkCmdBeginRendering>(
      vkGetDeviceProcAddr(device_.vk_device(), "vkCmdBeginRendering"));
  cmd_end_rendering_ = std::bit_cast<PFN_vkCmdEndRendering>(
      vkGetDeviceProcAddr(device_.vk_device(), "vkCmdEndRendering"));
  cmd_pipeline_barrier2_ = std::bit_cast<PFN_vkCmdPipelineBarrier2>(
      vkGetDeviceProcAddr(device_.vk_device(), "vkCmdPipelineBarrier2"));

  resolve_render_path();

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.vk_physical_device(), &properties);
  QUARK_LOG_INFO("Selected GPU: {} (physical device api {}.{}.{})",
                 properties.deviceName,
                 VK_VERSION_MAJOR(device_.capabilities().api_version),
                 VK_VERSION_MINOR(device_.capabilities().api_version),
                 VK_VERSION_PATCH(device_.capabilities().api_version));

  QUARK_OK();
}

util::Status VulkanContext::create_instance() {
  const bool enable_validation_layers =
      kEnableValidationLayers && check_validation_layer_support();

  if (kEnableValidationLayers && !enable_validation_layers) {
    QUARK_LOG_WARN("Validation layers requested, but unavailable. Continuing "
                   "without them.");
  }

  InstanceBundle::CreateInfo ci{};
  ci.instance.app_name = "quark-engine";
  ci.instance.engine_name = "quark";
  ci.instance.api_version = choose_instance_api_version();
  ci.instance.enable_debug_messenger = kEnableValidationLayers;

#if !QUARK_HEADLESS
  const auto *surface = platform::query<IVulkanSurfaceSource>(*window_);
  QUARK_ENSURE(surface != nullptr,
               QUARK_ERR(util::Errc::Unsupported,
                         "Window does not provide IVulkanSurfaceSource"));

  ci.instance.extensions = surface->required_instance_extensions();
#else
  ci.instance.extensions = {};
#endif

  QUARK_TRY_STATUS(instance_.create(ci));
  QUARK_OK();
}

#if !QUARK_HEADLESS

void VulkanContext::create_window() {
  auto window = std::make_unique<platform::GlfwWindow>();

  platform::IWindow::CreateInfo ci{};
  ci.width = kWindowWidth;
  ci.height = kWindowHeight;
  ci.title = kWindowTitle.data();
  ci.resizable = true;

  window->create(ci);
  window_ = std::move(window);
}

util::Status VulkanContext::create_presenter() {
  PresenterBundle::CreateInfo ci{};
  ci.instance = instance_.vk_instance();
  ci.physical_device = device_.vk_physical_device();
  ci.device = device_.vk_device();
  ci.graphics_queue_family_index = device_.graphics_queue_family_index();
  ci.present_queue_family_index = device_.present_queue_family_index();
  ci.window = window_.get();

  QUARK_TRY_STATUS(presenter_.create(ci));
  QUARK_OK();
}

util::Status VulkanContext::create_frame() {
  FrameBundle::CreateInfo ci{};
  ci.device = device_.view();
  ci.retire_queue = &retirement_queue_;
  ci.frames_in_flight = kMaxFramesInFlight;
  ci.cmd_buffer_count =
      static_cast<uint32_t>(presenter_.swapchain().images().size());
  ci.cmd_pool_flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  QUARK_TRY_STATUS(frame_.create(ci));
  QUARK_OK();
}

util::Status VulkanContext::create_render_pass() {
  if (render_path_ == RenderPath::Vulkan13DynamicRendering) {
    QUARK_OK();
  }

  VkAttachmentDescription color_attachment{};
  color_attachment.format = presenter_.swapchain().format();
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments = &color_attachment;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies = &dependency;

  QUARK_VK_TRY(vkCreateRenderPass(device_.vk_device(), &render_pass_info,
                                  nullptr, &render_pass_));
  QUARK_OK();
}

util::Status VulkanContext::create_framebuffers() {
  if (render_path_ == RenderPath::Vulkan13DynamicRendering) {
    framebuffers_.clear();
    QUARK_OK();
  }

  const auto &image_views = presenter_.swapchain().image_views();
  framebuffers_.resize(image_views.size());

  for (auto index{0U}; index < image_views.size(); ++index) {
    const array<VkImageView, 1> attachments = {image_views[index]};

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = presenter_.swapchain().extent().width;
    framebuffer_info.height = presenter_.swapchain().extent().height;
    framebuffer_info.layers = 1;

    QUARK_VK_TRY(vkCreateFramebuffer(device_.vk_device(), &framebuffer_info,
                                     nullptr, &framebuffers_[index]));
  }

  QUARK_OK();
}

util::Status VulkanContext::record_command_buffer(uint32_t image_index) {
  VkCommandBuffer cb = frame_.cmd()->cmd(image_index);

  QUARK_VK_TRY(vkResetCommandBuffer(cb, /*flags=*/0));

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  QUARK_VK_TRY(vkBeginCommandBuffer(cb, &begin_info));

  VkClearValue clear_color{};
  clear_color.color = {{0.08F, 0.08F, 0.1F, 1.0F}};

  if (render_path_ == RenderPath::Vulkan13DynamicRendering) {
    auto *image = presenter_.swapchain().images()[image_index];
    const VkImageLayout old_layout = swapchain_images_initialized_[image_index]
                                         ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                         : VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageMemoryBarrier2 to_color{};
    to_color.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    to_color.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    to_color.srcAccessMask = VK_ACCESS_2_NONE;
    to_color.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    to_color.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    to_color.oldLayout = old_layout;
    to_color.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    to_color.image = image;
    to_color.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_color.subresourceRange.baseMipLevel = 0;
    to_color.subresourceRange.levelCount = 1;
    to_color.subresourceRange.baseArrayLayer = 0;
    to_color.subresourceRange.layerCount = 1;

    VkDependencyInfo to_color_dependency{};
    to_color_dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    to_color_dependency.imageMemoryBarrierCount = 1;
    to_color_dependency.pImageMemoryBarriers = &to_color;
    cmd_pipeline_barrier2_(cb, &to_color_dependency);

    VkRenderingAttachmentInfo color_attachment{};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment.imageView =
        presenter_.swapchain().image_views()[image_index];
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue = clear_color;

    VkRenderingInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.offset = {.x = 0, .y = 0};
    rendering_info.renderArea.extent = presenter_.swapchain().extent();
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;

    cmd_begin_rendering_(cb, &rendering_info);
    cmd_end_rendering_(cb);

    VkImageMemoryBarrier2 to_present{};
    to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    to_present.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    to_present.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    to_present.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
    to_present.dstAccessMask = VK_ACCESS_2_NONE;
    to_present.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    to_present.image = image;
    to_present.subresourceRange = to_color.subresourceRange;

    VkDependencyInfo to_present_dependency{};
    to_present_dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    to_present_dependency.imageMemoryBarrierCount = 1;
    to_present_dependency.pImageMemoryBarriers = &to_present;
    cmd_pipeline_barrier2_(cb, &to_present_dependency);
  } else {
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_;
    render_pass_info.framebuffer = framebuffers_[image_index];
    render_pass_info.renderArea.offset = {.x = 0, .y = 0};
    render_pass_info.renderArea.extent = presenter_.swapchain().extent();
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(cb, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(cb);
  }

  QUARK_VK_TRY(vkEndCommandBuffer(cb));
  QUARK_OK();
}

util::Status VulkanContext::submit_frame(VkCommandBuffer command_buffer,
                                         VkSemaphore image_available,
                                         VkSemaphore render_finished,
                                         uint64_t signal_value) {
  if (render_path_ == RenderPath::Vulkan13DynamicRendering) {
    VkSemaphoreSubmitInfo wait_sem{};
    wait_sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait_sem.semaphore = image_available;
    wait_sem.value = 0;
    wait_sem.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    wait_sem.deviceIndex = 0;

    VkSemaphoreSubmitInfo signal_sem_bin{};
    signal_sem_bin.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal_sem_bin.semaphore = render_finished;
    signal_sem_bin.value = 0;
    signal_sem_bin.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    signal_sem_bin.deviceIndex = 0;

    VkSemaphoreSubmitInfo signal_sem_tl{};
    signal_sem_tl.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal_sem_tl.semaphore = gpu_timeline_.semaphore();
    signal_sem_tl.value = signal_value;
    signal_sem_tl.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    signal_sem_tl.deviceIndex = 0;

    VkCommandBufferSubmitInfo cb_info{};
    cb_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cb_info.commandBuffer = command_buffer;
    cb_info.deviceMask = 0;

    const std::array<VkSemaphoreSubmitInfo, 2> signals = {signal_sem_bin,
                                                          signal_sem_tl};

    VkSubmitInfo2 submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = &wait_sem;
    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &cb_info;
    submit_info.signalSemaphoreInfoCount =
        static_cast<uint32_t>(signals.size());
    submit_info.pSignalSemaphoreInfos = signals.data();

    QUARK_VK_TRY(queue_submit2_(device_.graphics_queue(), /*submitCount=*/1,
                                &submit_info, /*fence=*/VK_NULL_HANDLE));
    QUARK_OK();
  }

  const VkPipelineStageFlags wait_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  const std::array<VkSemaphore, 2> signal_semaphores = {
      render_finished, gpu_timeline_.semaphore()};
  const std::array<uint64_t, 2> signal_values = {0, signal_value};

  VkTimelineSemaphoreSubmitInfo timeline_info{};
  timeline_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timeline_info.waitSemaphoreValueCount = 0;
  timeline_info.pWaitSemaphoreValues = nullptr;
  timeline_info.signalSemaphoreValueCount =
      static_cast<uint32_t>(signal_values.size());
  timeline_info.pSignalSemaphoreValues = signal_values.data();

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = &timeline_info;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_available;
  submit_info.pWaitDstStageMask = &wait_stage;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;
  submit_info.signalSemaphoreCount =
      static_cast<uint32_t>(signal_semaphores.size());
  submit_info.pSignalSemaphores = signal_semaphores.data();

  QUARK_VK_TRY(vkQueueSubmit(device_.graphics_queue(), /*submitCount=*/1,
                             &submit_info, /*fence=*/VK_NULL_HANDLE));
  QUARK_OK();
}

util::Status VulkanContext::draw_frame() {
#if QUARK_HEADLESS
  QUARK_FAIL(QUARK_ERR(util::Errc::Unsupported,
                       "draw_frame is unavailable in headless mode"));
#endif

  std::size_t drained = 0;
  QUARK_TRY_ASSIGN(drained, retirement_queue_.drain());
  (void)drained;

  auto view = frame_.view();

  // Wait until this frame slot is free
  const uint64_t frame_value = view.sync->in_flight_value(current_frame_);
  QUARK_TRY_STATUS(gpu_timeline_.wait(frame_value));

  // Acquire image
  uint32_t image_index = 0;
  const VkResult acquire_result = vkAcquireNextImageKHR(
      device_.vk_device(), presenter_.swapchain().handle(), UINT64_MAX,
      view.sync->image_available(current_frame_), VK_NULL_HANDLE, &image_index);
  if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
    QUARK_TRY_STATUS(recreate_swapchain());
    QUARK_OK();
  }
  if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
    QUARK_FAIL(::quark::vk::vk_error(acquire_result, "vkAcquireNextImageKHR"));
  }

  // If this swapchain image is used by an earlier frame, wait for it
  if (image_index < images_in_flight_.size()) {
    const uint64_t image_value = images_in_flight_[image_index];
    QUARK_TRY_STATUS(gpu_timeline_.wait(image_value));
  }

  QUARK_TRY_STATUS(record_command_buffer(image_index));

  const uint64_t signal_value = gpu_timeline_.next_signal_value();

  VkCommandBuffer cb = view.cmd->cmd(image_index);
  QUARK_TRY_STATUS(submit_frame(cb, view.sync->image_available(current_frame_),
                                view.sync->render_finished(current_frame_),
                                signal_value));

  // REMOVE TEST
  // static uint64_t retire_test_id = 1;
  // QUARK_TRY_STATUS(
  //     enqueue_retire_test(retirement_queue_, signal_value,
  //     retire_test_id++));

  view.sync->mark_submitted(current_frame_, signal_value);
  images_in_flight_[image_index] = signal_value;
  swapchain_images_initialized_[image_index] = true;

  const array<VkSwapchainKHR, 1> swapchains = {presenter_.swapchain().handle()};
  const array<VkSemaphore, 1> present_wait = {
      view.sync->render_finished(current_frame_)};

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = present_wait.data();
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains.data();
  present_info.pImageIndices = &image_index;

  const VkResult present_result =
      vkQueuePresentKHR(device_.present_queue(), &present_info);

  if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
      present_result == VK_SUBOPTIMAL_KHR) {
    QUARK_TRY_STATUS(recreate_swapchain());
  } else if (present_result != VK_SUCCESS) {
    QUARK_FAIL(::quark::vk::vk_error(present_result, "vkQueuePresentKHR"));
  }

  current_frame_ = (current_frame_ + 1U) % kMaxFramesInFlight;
  QUARK_OK();
}

void VulkanContext::cleanup_swapchain() {
  for (VkFramebuffer framebuffer : framebuffers_) {
    if (framebuffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device_.vk_device(), framebuffer, nullptr);
    }
  }
  framebuffers_.clear();

  if (render_pass_ != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device_.vk_device(), render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }

  presenter_.destroy_swapchain();
  swapchain_images_initialized_.clear();
}

util::Status VulkanContext::recreate_swapchain() {
  int width{0};
  int height{0};
  while (width == 0 || height == 0) {
    window_->framebuffer_size(width, height);
    window_->wait_events();
  }

  vkDeviceWaitIdle(device_.vk_device());

  cleanup_swapchain();
  QUARK_TRY_STATUS(presenter_.recreate_swapchain());

  images_in_flight_.assign(presenter_.swapchain().images().size(), 0);
  swapchain_images_initialized_.assign(presenter_.swapchain().images().size(),
                                       false);

  QUARK_TRY_STATUS(create_render_pass());
  QUARK_TRY_STATUS(create_framebuffers());

  QUARK_TRY_STATUS(frame_.cmd()->resize(
      static_cast<uint32_t>(presenter_.swapchain().images().size())));
  QUARK_TRY_STATUS(frame_.sync()->resize(
      static_cast<uint32_t>(presenter_.swapchain().images().size())));

  QUARK_OK();
}

#endif

void VulkanContext::resolve_render_path() {
  const auto caps = device_.capabilities();

  const bool can_use_dynamic_rendering =
      api_version_at_least(caps.api_version, 1, 3) &&
      caps.enabled(details::DeviceFeature::DynamicRendering) &&
      caps.enabled(details::DeviceFeature::Synchronization2) &&
      queue_submit2_ != nullptr && cmd_begin_rendering_ != nullptr &&
      cmd_end_rendering_ != nullptr && cmd_pipeline_barrier2_ != nullptr;

  render_path_ = can_use_dynamic_rendering
                     ? RenderPath::Vulkan13DynamicRendering
                     : RenderPath::Vulkan12Fallback;

  QUARK_LOG_INFO("render path selected: {}",
                 render_path_ == RenderPath::Vulkan13DynamicRendering
                     ? "vk13 dynamic rendering"
                     : "vk12 render pass fallback");
}

util::Status VulkanContext::create_gpu_timeline() {
  GpuTimeline::CreateInfo ci{};
  ci.device = device_.view();
  QUARK_TRY_STATUS(gpu_timeline_.create(ci));
  QUARK_OK();
}

util::Status VulkanContext::create_retirement_queue() {
  RetirementQueue::CreateInfo ci{};
  ci.timeline = &gpu_timeline_;
  ci.reserve = 256; // TODO: tune later
  QUARK_TRY_STATUS(retirement_queue_.create(ci));
  QUARK_OK();
}

} // namespace quark::vk
