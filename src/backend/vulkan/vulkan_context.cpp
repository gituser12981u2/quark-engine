#include "vulkan_context.hpp"
#include "quark/platform/window/IWindow.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/details/device.hpp"
#include "quark/vk/instance/details/instance.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <limits>
#include <memory>
#include <quark/platform/window/glfw_window.hpp>
#include <quark/platform/window/interface_query.hpp>
#include <quark/vk/instance/instance_bundle.hpp>
#include <quark/vk/surface_source.hpp>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

using std::array;
using std::vector;

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
  create_command_pool();
  create_swapchain();
  create_swapchain_image_views();
  create_render_pass();
  create_framebuffers();
  create_command_buffers();
  create_sync_objects();

  return {};
}

VulkanContext::~VulkanContext() {
  if (device_.valid()) {
    VkDevice logical_device = device_.vk_device();
    vkDeviceWaitIdle(logical_device);

    for (auto index{0UZ}; index < kMaxFramesInFlight; ++index) {
      if (image_available_semaphores_[index] != VK_NULL_HANDLE) {
        vkDestroySemaphore(logical_device, image_available_semaphores_[index],
                           nullptr);
      }
      if (in_flight_fences_[index] != VK_NULL_HANDLE) {
        vkDestroyFence(logical_device, in_flight_fences_[index], nullptr);
      }
    }

    cleanup_swapchain();

    if (command_pool_ != VK_NULL_HANDLE) {
      vkDestroyCommandPool(logical_device, command_pool_, nullptr);
    }

    device_.destroy();
  }

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
    draw_frame();
  }

  if (device_.valid()) {
    vkDeviceWaitIdle(device_.vk_device());
  }

  return {};
}

void VulkanContext::create_window() {
  window_ = std::make_unique<platform::GlfwWindow>();

  platform::IWindow::CreateInfo ci{};
  ci.width = kWindowWidth;
  ci.height = kWindowHeight;
  ci.title = kWindowTitle.data();
  ci.resizable = true;

  window_->create(ci);
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

  Instance::CreateInfo ci{};
  ci.app_name = "quark-engine";
  ci.engine_name = "quark";
  ci.api_version = VK_API_VERSION_1_3;
  ci.enable_debug_messenger = true;
  ci.extensions = surface->required_instance_extensions();

  QUARK_TRY_STATUS(instance_.create(ci));
  return {};
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
  Device::CreateInfo ci{};
  ci.instance = instance_.vk_instance();
  ci.surface = surface_;
  ci.required_extensions = {kSwapchainExtension};
  QUARK_TRY_STATUS(device_.create(ci));

  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device_.vk_physical_device(), &properties);
  QUARK_LOG_INFO("Selected GPU: {}", properties.deviceName);

  return {};
}

void VulkanContext::create_command_pool() {
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = device_.graphics_queue_family_index();

  throw_if_vk_failed(vkCreateCommandPool(device_.vk_device(), &pool_info,
                                         nullptr, &command_pool_),
                     "vkCreateCommandPool");
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

void VulkanContext::create_command_buffers() {
  command_buffers_.resize(framebuffers_.size());

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount =
      static_cast<uint32_t>(command_buffers_.size());

  throw_if_vk_failed(vkAllocateCommandBuffers(device_.vk_device(), &alloc_info,
                                              command_buffers_.data()),
                     "vkAllocateCommandBuffers");

  for (auto index{0U}; index < command_buffers_.size(); ++index) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    throw_if_vk_failed(
        vkBeginCommandBuffer(command_buffers_[index], &begin_info),
        "vkBeginCommandBuffer");

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_;
    render_pass_info.framebuffer = framebuffers_[index];
    render_pass_info.renderArea.offset = {.x = 0, .y = 0};
    render_pass_info.renderArea.extent = swapchain_extent_;

    VkClearValue clear_color{};
    clear_color.color = {{0.08F, 0.08F, 0.1F, 1.0F}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffers_[index], &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(command_buffers_[index]);

    throw_if_vk_failed(vkEndCommandBuffer(command_buffers_[index]),
                       "vkEndCommandBuffer");
  }
}

void VulkanContext::create_sync_objects() {
  images_in_flight_.assign(swapchain_images_.size(), VK_NULL_HANDLE);
  render_finished_semaphores_per_image_.assign(swapchain_images_.size(),
                                               VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t index = 0; index < kMaxFramesInFlight; ++index) {
    throw_if_vk_failed(vkCreateSemaphore(device_.vk_device(), &semaphore_info,
                                         nullptr,
                                         &image_available_semaphores_[index]),
                       "vkCreateSemaphore(image available)");
    throw_if_vk_failed(vkCreateFence(device_.vk_device(), &fence_info, nullptr,
                                     &in_flight_fences_[index]),
                       "vkCreateFence");
  }

  for (VkSemaphore &render_finished_semaphore :
       render_finished_semaphores_per_image_) {
    throw_if_vk_failed(vkCreateSemaphore(device_.vk_device(), &semaphore_info,
                                         nullptr, &render_finished_semaphore),
                       "vkCreateSemaphore(render finished per image)");
  }
}

void VulkanContext::draw_frame() {
  throw_if_vk_failed(vkWaitForFences(device_.vk_device(), 1,
                                     &in_flight_fences_[current_frame_],
                                     VK_TRUE, UINT64_MAX),
                     "vkWaitForFences");

  uint32_t image_index = 0;
  const VkResult acquire_result =
      vkAcquireNextImageKHR(device_.vk_device(), swapchain_, UINT64_MAX,
                            image_available_semaphores_[current_frame_],
                            VK_NULL_HANDLE, &image_index);

  if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreate_swapchain();
    return;
  }

  if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
    throw_if_vk_failed(acquire_result, "vkAcquireNextImageKHR");
  }

  if (images_in_flight_[image_index] != VK_NULL_HANDLE) {
    throw_if_vk_failed(vkWaitForFences(device_.vk_device(), 1,
                                       &images_in_flight_[image_index], VK_TRUE,
                                       UINT64_MAX),
                       "vkWaitForFences(image)");
  }
  images_in_flight_[image_index] = in_flight_fences_[current_frame_];

  throw_if_vk_failed(
      vkResetFences(device_.vk_device(), 1, &in_flight_fences_[current_frame_]),
      "vkResetFences");

  const array<VkSemaphore, 1> wait_semaphores = {
      image_available_semaphores_[current_frame_]};
  const array<VkPipelineStageFlags, 1> wait_stages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  const array<VkSemaphore, 1> signal_semaphores = {
      render_finished_semaphores_per_image_[image_index]};

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores.data();
  submit_info.pWaitDstStageMask = wait_stages.data();
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffers_[image_index];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores.data();

  throw_if_vk_failed(vkQueueSubmit(device_.graphics_queue(), 1, &submit_info,
                                   in_flight_fences_[current_frame_]),
                     "vkQueueSubmit");

  const array<VkSwapchainKHR, 1> swapchains = {swapchain_};
  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores.data();
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swapchains.data();
  present_info.pImageIndices = &image_index;

  const VkResult present_result =
      vkQueuePresentKHR(device_.present_queue(), &present_info);

  if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
      present_result == VK_SUBOPTIMAL_KHR) {
    recreate_swapchain();
  } else {
    throw_if_vk_failed(present_result, "vkQueuePresentKHR");
  }

  current_frame_ = (current_frame_ + 1U) % kMaxFramesInFlight;
}

void VulkanContext::cleanup_swapchain() {
  for (auto *semaphore : render_finished_semaphores_per_image_) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device_.vk_device(), semaphore, nullptr);
    }
  }
  render_finished_semaphores_per_image_.clear();

  for (auto *framebuffer : framebuffers_) {
    vkDestroyFramebuffer(device_.vk_device(), framebuffer, nullptr);
  }
  framebuffers_.clear();

  if (!command_buffers_.empty()) {
    vkFreeCommandBuffers(device_.vk_device(), command_pool_,
                         static_cast<uint32_t>(command_buffers_.size()),
                         command_buffers_.data());
    command_buffers_.clear();
  }

  if (render_pass_ != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device_.vk_device(), render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }

  for (auto *image_view : swapchain_image_views_) {
    vkDestroyImageView(device_.vk_device(), image_view, nullptr);
  }
  swapchain_image_views_.clear();

  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_.vk_device(), swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  swapchain_images_.clear();
}

void VulkanContext::recreate_swapchain() {
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
  create_render_pass();
  create_framebuffers();
  create_command_buffers();

  images_in_flight_.assign(swapchain_images_.size(), VK_NULL_HANDLE);
  render_finished_semaphores_per_image_.assign(swapchain_images_.size(),
                                               VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (auto index{0U}; index < render_finished_semaphores_per_image_.size();
       ++index) {
    throw_if_vk_failed(
        vkCreateSemaphore(device_.vk_device(), &semaphore_info, nullptr,
                          &render_finished_semaphores_per_image_[index]),
        "vkCreateSemaphore(render finished per image recreate)");
  }
}

} // namespace quark::vk
