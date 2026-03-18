#include <algorithm>
#include <array>
#include <limits>
#include <quark/platform/window/IWindow.hpp>
#include <quark/vk/presentation/details/swapchain.hpp>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

namespace {

struct SupportDetails {
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

SupportDetails query_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
  SupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t format_count{0};
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                       /*pSurfaceFormats=*/nullptr);
  if (format_count != 0) {
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                         details.formats.data());
  }

  uint32_t present_mode_count{0};
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      device, surface, &present_mode_count, /*pPresentModes=*/nullptr);
  if (present_mode_count != 0) {
    details.present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &present_mode_count, details.present_modes.data());
  }

  return details;
}

VkSurfaceFormatKHR
choose_format(const std::vector<VkSurfaceFormatKHR> &available_formats,
              VkFormat preferred_format, VkColorSpaceKHR preferred_cs) {
  for (const VkSurfaceFormatKHR &surface_format : available_formats) {
    if (surface_format.format == preferred_format &&
        surface_format.colorSpace == preferred_cs) {
      return surface_format;
    }
  }

  return available_formats.front();
}

VkPresentModeKHR choose_present_mode(
    const std::vector<VkPresentModeKHR> &available_present_modes,
    VkPresentModeKHR preferred) {
  for (const VkPresentModeKHR present_mode : available_present_modes) {
    if (present_mode == preferred) {
      return present_mode;
    }
  }

  // FIFO is guaranteed
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_extent(const platform::IWindow &window,
                         const VkSurfaceCapabilitiesKHR &capabilities) {
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

} // namespace

Swapchain::Swapchain(Swapchain &&other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      swapchain_(std::exchange(other.swapchain_, VK_NULL_HANDLE)),
      image_format_(std::exchange(other.image_format_, VK_FORMAT_UNDEFINED)),
      extent_(std::exchange(other.extent_, VkExtent2D{})),
      images_(std::move(other.images_)),
      image_views_(std::move(other.image_views_)) {}

Swapchain &Swapchain::operator=(Swapchain &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  reset();

  device_ = std::exchange(other.device_, VK_NULL_HANDLE);
  swapchain_ = std::exchange(other.swapchain_, VK_NULL_HANDLE);
  image_format_ = std::exchange(other.image_format_, VK_FORMAT_UNDEFINED);
  extent_ = std::exchange(other.extent_, VkExtent2D{});
  images_ = std::move(other.images_);
  image_views_ = std::move(other.image_views_);
  return *this;
}

void Swapchain::create(const CreateInfo &ci) {
  reset();

  // TODO: Replace with device valid()
  if (ci.physical_device == VK_NULL_HANDLE) {
    throw std::runtime_error("Swapchain::create: physical_device is null");
  }

  if (ci.device == VK_NULL_HANDLE) {
    throw std::runtime_error("Swapchain::create: device is null");
  }

  if (ci.surface == VK_NULL_HANDLE) {
    throw std::runtime_error("Swapchain::create: surface is null");
  }

  // TODO: Replace with window valid
  if (ci.window == nullptr) {
    throw std::runtime_error("Swapchain::create: window is null");
  }

  device_ = ci.device;

  const SupportDetails support = query_support(ci.physical_device, ci.surface);
  if (support.formats.empty() || support.present_modes.empty()) {
    throw std::runtime_error(
        "Swapchain::create: swapchain support is incomplete");
  }

  const VkSurfaceFormatKHR surface_format = choose_format(
      support.formats, ci.preferred_format, ci.preferred_color_space);
  const VkPresentModeKHR present_mode =
      choose_present_mode(support.present_modes, ci.preferred_present_mode);
  const VkExtent2D extent = choose_extent(*ci.window, support.capabilities);

  uint32_t image_count = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0U &&
      image_count > support.capabilities.maxImageCount) {
    image_count = support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = ci.surface;
  sci.minImageCount = image_count;
  sci.imageFormat = surface_format.format;
  sci.imageColorSpace = surface_format.colorSpace;
  sci.imageExtent = extent;
  sci.imageArrayLayers = 1;
  sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  sci.preTransform = support.capabilities.currentTransform;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sci.presentMode = present_mode;
  sci.clipped = VK_TRUE;
  sci.oldSwapchain = ci.old_swapchain;

  const std::array<uint32_t, 2> queue_family_indices = {
      ci.graphics_queue_family_index, ci.present_queue_family_index};

  if (ci.graphics_queue_family_index != ci.present_queue_family_index) {
    sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    sci.queueFamilyIndexCount = 2;
    sci.pQueueFamilyIndices = queue_family_indices.data();
  } else {
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  VkResult r = vkCreateSwapchainKHR(ci.device, &sci, /*pAllocator=*/nullptr,
                                    &swapchain_);
  if (r != VK_SUCCESS) {
    throw std::runtime_error("vkCreateSwapchainKHR failed");
  }

  vkGetSwapchainImagesKHR(ci.device, swapchain_, &image_count,
                          /*pSwapchainImages=*/nullptr);
  images_.resize(image_count);
  vkGetSwapchainImagesKHR(ci.device, swapchain_, &image_count, images_.data());

  image_format_ = surface_format.format;
  extent_ = extent;

  image_views_.resize(images_.size(), VK_NULL_HANDLE);
  for (auto index{0U}; index < images_.size(); ++index) {
    VkImageViewCreateInfo ivci{};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = images_[index];
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = image_format_;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;

    r = vkCreateImageView(ci.device, &ivci, /*pAllocator=*/nullptr,
                          &image_views_[index]);
    if (r != VK_SUCCESS) {
      throw std::runtime_error("vkCreateImageView failed");
    }
  }
}

void Swapchain::reset() noexcept {
  // Destroy views
  if (device_ != VK_NULL_HANDLE) {
    for (VkImageView v : image_views_) {
      if (v != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, v, /*pAllocator=*/nullptr);
      }
    }
  }

  image_views_.clear();
  images_.clear();

  if (device_ != VK_NULL_HANDLE && swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, swapchain_, /*pAllocator=*/nullptr);
  }

  swapchain_ = VK_NULL_HANDLE;
  image_format_ = VK_FORMAT_UNDEFINED;
  extent_ = VkExtent2D{};
  device_ = VK_NULL_HANDLE;
}

} // namespace quark::vk
