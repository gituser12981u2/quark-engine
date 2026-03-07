#include "vulkan_context.hpp"

// TODO: move test to testing system when possible
#include "quark/engine/retire/retirement_queue.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <limits>
#include <memory>
#include <new>
#include <quark/platform/window/glfw_window.hpp>
#include <quark/platform/window/interface_query.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/instance/instance_bundle.hpp>
#include <quark/vk/surface_source.hpp>
#include <quark/vk/sync/gpu_timeline.hpp>
#include <stdexcept>
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

util::Status enqueue_retire_test(quark::vk::RetirementQueue &queue,
                                 uint64_t retire_at, uint64_t id) {
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

constexpr int kWindowWidth{1280};
constexpr int kWindowHeight{720};
constexpr std::string_view kWindowTitle = "quark-engine";

constexpr bool kEnableValidationLayers =
#ifndef NDEBUG
    true;
#else
    false;
#endif

constexpr array<const char *, 1> kValidationLayers{
    "VK_LAYER_KHRONOS_validation",
};

constexpr const char *kSwapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  vector<VkSurfaceFormatKHR> formats;
  vector<VkPresentModeKHR> present_modes;
};

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

auto query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
    -> SwapchainSupportDetails {
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t format_count{0};
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
  if (format_count > 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                         details.formats.data());
  }

  uint32_t present_mode_count{0};
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                            &present_mode_count, nullptr);
  if (present_mode_count > 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &present_mode_count, details.present_modes.data());
  }

  return details;
}

auto choose_swapchain_surface_format(
    const vector<VkSurfaceFormatKHR> &available_formats) -> VkSurfaceFormatKHR {
  for (const VkSurfaceFormatKHR &surface_format : available_formats) {
    if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return surface_format;
    }
  }
  return available_formats.front();
}

auto choose_swapchain_present_mode(
    const vector<VkPresentModeKHR> &available_present_modes)
    -> VkPresentModeKHR {
  for (const VkPresentModeKHR present_mode : available_present_modes) {
    if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return present_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

auto choose_swapchain_extent(const quark::platform::IWindow &window,
                             const VkSurfaceCapabilitiesKHR &capabilities)
    -> VkExtent2D {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  int width{0};
  int height{0};
  window.framebuffer_size(width, height);

  VkExtent2D actual_extent{
      .width = static_cast<uint32_t>(width),
      .height = static_cast<uint32_t>(height),
  };

  actual_extent.width =
      std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                 capabilities.maxImageExtent.width);
  actual_extent.height =
      std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                 capabilities.maxImageExtent.height);
  return actual_extent;
}

auto vk_result_to_string(const VkResult result) -> std::string_view {
  switch (result) {
  case VK_SUCCESS:
    return "VK_SUCCESS";
  case VK_ERROR_LAYER_NOT_PRESENT:
    return "VK_ERROR_LAYER_NOT_PRESENT";
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return "VK_ERROR_EXTENSION_NOT_PRESENT";
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return "VK_ERROR_INCOMPATIBLE_DRIVER";
  case VK_ERROR_OUT_OF_DATE_KHR:
    return "VK_ERROR_OUT_OF_DATE_KHR";
  default:
    return "VK_UNKNOWN_ERROR";
  }
}

void throw_if_vk_failed(VkResult result, std::string_view step) {
  if (result != VK_SUCCESS) {
    throw std::runtime_error(
        std::format("{} failed with {}", step, vk_result_to_string(result)));
  }
}

} // namespace

namespace quark::vk {

util::Status VulkanContext::init() {
  create_window();

  QUARK_TRY_STATUS(create_instance());
  create_surface();

  QUARK_TRY_STATUS(create_device());
  QUARK_TRY_STATUS(create_gpu_timeline());
  QUARK_TRY_STATUS(create_retirement_queue());

  create_swapchain();
  create_swapchain_image_views();

  images_in_flight_.assign(swapchain_images_.size(), 0);

  QUARK_TRY_STATUS(create_frame());

  create_render_pass();
  create_framebuffers();

  QUARK_TRY_STATUS(record_command_buffers());

  return {};
}

VulkanContext::~VulkanContext() {
  if (device_.validate()) {
    VkDevice device = device_.vk_device();
    vkDeviceWaitIdle(device);
  }

  frame_.destroy();
  retirement_queue_.destroy();
  gpu_timeline_.destroy();

  cleanup_swapchain();
  device_.destroy();

  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance_.vk_instance(), surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }

  instance_.destroy();
}

util::Status VulkanContext::run() {
  QUARK_TRY_STATUS(init());
  QUARK_LOG_INFO("Vulkan initialised successfully.");

  while (!window_->should_close()) {
    window_->poll_events();
    QUARK_TRY_STATUS(draw_frame());
  }

  if (device_.validate()) {
    vkDeviceWaitIdle(device_.vk_device());
  }

  QUARK_OK();
}

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

util::Status VulkanContext::create_instance() {
  const bool enable_validation_layers =
      kEnableValidationLayers && check_validation_layer_support();

  if (kEnableValidationLayers && !enable_validation_layers) {
    QUARK_LOG_WARN("Validation layers requested, but unavailable. Continuing "
                   "without them.");
  }

  const auto *surface = platform::query<IVulkanSurfaceSource>(*window_);
  QUARK_ENSURE(surface != nullptr,
               QUARK_ERR(util::Errc::Unsupported,
                         "Window does not provide IVulkanSurfaceSource"));

  InstanceBundle::CreateInfo ci{};
  ci.instance.app_name = "quark-engine";
  ci.instance.engine_name = "quark";
  ci.instance.api_version = VK_API_VERSION_1_3;
  ci.instance.enable_debug_messenger = true;
  ci.instance.extensions = surface->required_instance_extensions();

  QUARK_TRY_STATUS(instance_.create(ci));

  QUARK_OK();
}

void VulkanContext::create_surface() {
  const auto *surface = platform::query<vk::IVulkanSurfaceSource>(*window_);
  if (surface == nullptr) {
    throw std::runtime_error("Window does not provide IVulkanSurfaceSource");
  }

  surface_ = surface->create_surface(instance_.vk_instance());
  if (surface_ == VK_NULL_HANDLE) {
    throw std::runtime_error("create_surface returned VK_NULL_HANDLE");
  }
}

util::Status VulkanContext::create_device() {
  DeviceBundle::CreateInfo ci{};
  ci.device.instance = instance_.vk_instance();
  ci.device.surface = surface_;
  ci.device.requested_features = static_cast<details::Device::FeatureFlags>(
      details::Device::Features::TimelineSemaphore);
  ci.device.required_extensions = {kSwapchainExtension};
  QUARK_TRY_STATUS(device_.create(ci));

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.vk_physical_device(), &properties);
  QUARK_LOG_INFO("Selected GPU: {}", properties.deviceName);

  QUARK_OK();
}

util::Status VulkanContext::create_gpu_timeline() {
  GpuTimeline::CreateInfo ci{};
  ci.device = device_.view();
  QUARK_TRY_STATUS(gpu_timeline_.create(ci));
  QUARK_OK();
}

void VulkanContext::create_swapchain() {
  const SwapchainSupportDetails swapchain_support =
      query_swapchain_support(device_.vk_physical_device(), surface_);

  const VkSurfaceFormatKHR surface_format =
      choose_swapchain_surface_format(swapchain_support.formats);
  const VkPresentModeKHR present_mode =
      choose_swapchain_present_mode(swapchain_support.present_modes);
  const VkExtent2D extent =
      choose_swapchain_extent(*window_, swapchain_support.capabilities);

  uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
  if (swapchain_support.capabilities.maxImageCount > 0U &&
      image_count > swapchain_support.capabilities.maxImageCount) {
    image_count = swapchain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface_;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  const array<uint32_t, 2> queue_family_indices = {
      device_.graphics_queue_family_index(),
      device_.present_queue_family_index()};

  if (device_.graphics_queue_family_index() !=
      device_.present_queue_family_index()) {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices.data();
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  create_info.preTransform = swapchain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  throw_if_vk_failed(vkCreateSwapchainKHR(device_.vk_device(), &create_info,
                                          nullptr, &swapchain_),
                     "vkCreateSwapchainKHR");

  vkGetSwapchainImagesKHR(device_.vk_device(), swapchain_, &image_count,
                          nullptr);
  swapchain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(device_.vk_device(), swapchain_, &image_count,
                          swapchain_images_.data());

  swapchain_image_format_ = surface_format.format;
  swapchain_extent_ = extent;
}

void VulkanContext::create_swapchain_image_views() {
  swapchain_image_views_.resize(swapchain_images_.size());

  for (auto index{0U}; index < swapchain_images_.size(); ++index) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swapchain_images_[index];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swapchain_image_format_;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    throw_if_vk_failed(vkCreateImageView(device_.vk_device(), &create_info,
                                         nullptr,
                                         &swapchain_image_views_[index]),
                       "vkCreateImageView");
  }
}

util::Status VulkanContext::create_frame() {
  FrameBundle::CreateInfo ci{};
  ci.device = device_.view();
  ci.retire_queue = &retirement_queue_;
  ci.frames_in_flight = kMaxFramesInFlight;
  ci.cmd_buffer_count = static_cast<uint32_t>(swapchain_images_.size());
  ci.cmd_pool_flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  QUARK_TRY_STATUS(frame_.create(ci));
  QUARK_OK();
}

util::Status VulkanContext::create_retirement_queue() {
  RetirementQueue::CreateInfo ci{};
  ci.timeline = &gpu_timeline_;
  ci.reserve = 256; // TODO: tune later
  QUARK_TRY_STATUS(retirement_queue_.create(ci));
  QUARK_OK();
}

void VulkanContext::create_render_pass() {
  VkAttachmentDescription color_attachment{};
  color_attachment.format = swapchain_image_format_;
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

  throw_if_vk_failed(vkCreateRenderPass(device_.vk_device(), &render_pass_info,
                                        nullptr, &render_pass_),
                     "vkCreateRenderPass");
}

void VulkanContext::create_framebuffers() {
  framebuffers_.resize(swapchain_image_views_.size());

  for (auto index{0U}; index < swapchain_image_views_.size(); ++index) {
    const array<VkImageView, 1> attachments = {swapchain_image_views_[index]};

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = swapchain_extent_.width;
    framebuffer_info.height = swapchain_extent_.height;
    framebuffer_info.layers = 1;

    throw_if_vk_failed(vkCreateFramebuffer(device_.vk_device(),
                                           &framebuffer_info, nullptr,
                                           &framebuffers_[index]),
                       "vkCreateFramebuffer");
  }
}

util::Status VulkanContext::record_command_buffers() {
  for (auto i{0U}; i < frame_.cmd()->count(); ++i) {
    VkCommandBuffer cb = frame_.cmd()->cmd(i);

    QUARK_VK_TRY(vkResetCommandBuffer(cb, /*flags=*/0));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    QUARK_VK_TRY(vkBeginCommandBuffer(cb, &begin_info));

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_;
    render_pass_info.framebuffer = framebuffers_[i];
    render_pass_info.renderArea.offset = {.x = 0, .y = 0};
    render_pass_info.renderArea.extent = swapchain_extent_;

    VkClearValue clear_color{};
    clear_color.color = {{0.08F, 0.08F, 0.1F, 1.0F}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(cb, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(cb);

    QUARK_VK_TRY(vkEndCommandBuffer(cb));
  }

  return {};
}

util::Status VulkanContext::draw_frame() {
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
      device_.vk_device(), swapchain_, UINT64_MAX,
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

  const uint64_t signal_value = gpu_timeline_.next_signal_value();

  VkSemaphore wait_bin = view.sync->image_available(current_frame_);
  VkSemaphore signal_bin = view.sync->render_finished(current_frame_);
  VkSemaphore signal_tl = gpu_timeline_.semaphore();

  VkSemaphoreSubmitInfo wait_sem{};
  wait_sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  wait_sem.semaphore = wait_bin;
  wait_sem.value = 0;
  wait_sem.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  wait_sem.deviceIndex = 0;

  VkSemaphoreSubmitInfo signal_sem_bin{};
  signal_sem_bin.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signal_sem_bin.semaphore = signal_bin;
  signal_sem_bin.value = 0;
  signal_sem_bin.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
  signal_sem_bin.deviceIndex = 0;

  VkSemaphoreSubmitInfo signal_sem_tl{};
  signal_sem_tl.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signal_sem_tl.semaphore = signal_tl;
  signal_sem_tl.value = signal_value;
  signal_sem_tl.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
  signal_sem_tl.deviceIndex = 0;

  VkCommandBuffer cb = view.cmd->cmd(image_index);

  VkCommandBufferSubmitInfo cb_info{};
  cb_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cb_info.commandBuffer = cb;
  cb_info.deviceMask = 0;

  std::array<VkSemaphoreSubmitInfo, 2> signals = {signal_sem_bin,
                                                  signal_sem_tl};

  VkSubmitInfo2 submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit_info.waitSemaphoreInfoCount = 1;
  submit_info.pWaitSemaphoreInfos = &wait_sem;
  submit_info.commandBufferInfoCount = 1;
  submit_info.pCommandBufferInfos = &cb_info;
  submit_info.signalSemaphoreInfoCount = static_cast<uint32_t>(signals.size());
  submit_info.pSignalSemaphoreInfos = signals.data();

  QUARK_VK_TRY(vkQueueSubmit2(device_.graphics_queue(), /*submitCount=*/1,
                              &submit_info,
                              /*fence=*/VK_NULL_HANDLE));

  // REMOVE TEST
  // static uint64_t retire_test_id = 1;
  // QUARK_TRY_STATUS(
  //     enqueue_retire_test(retirement_queue_, signal_value,
  //     retire_test_id++));

  view.sync->mark_submitted(current_frame_, signal_value);
  images_in_flight_[image_index] = signal_value;

  const array<VkSwapchainKHR, 1> swapchains = {swapchain_};
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

  for (VkImageView image_view : swapchain_image_views_) {
    if (image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(device_.vk_device(), image_view, nullptr);
    }
  }
  swapchain_image_views_.clear();

  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_.vk_device(), swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  swapchain_images_.clear();
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
  create_swapchain();
  create_swapchain_image_views();

  images_in_flight_.assign(swapchain_images_.size(), 0);

  create_render_pass();
  create_framebuffers();

  QUARK_TRY_STATUS(frame_.view().sync->resize(
      static_cast<uint32_t>(swapchain_images_.size())));
  QUARK_TRY_STATUS(record_command_buffers());

  QUARK_OK();
}

} // namespace quark::vk
