#include <quark/platform/window/interface_query.hpp>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/presentation/presenter_bundle.hpp>
#include <quark/vk/surface_source.hpp>

namespace quark::vk {

namespace {

util::Result<VkSurfaceKHR> create_surface(VkInstance instance,
                                          const platform::IWindow *window) {
  QUARK_ENSURE(instance != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg,
                         "PresenterBundle::create_surface: instance is null"));
  QUARK_ENSURE(window != nullptr,
               QUARK_ERR(util::Errc::InvalidArg,
                         "PresenterBundle::create_surface: window is null"));

  const auto *source = platform::query<IVulkanSurfaceSource>(*window);
  QUARK_ENSURE(
      source != nullptr,
      QUARK_ERR(util::Errc::Unsupported,
                "PresenterBundle::create_surface: window does not expose "
                "IVulkanSurfaceSource"));

  VkSurfaceKHR surface = source->create_surface(instance);
  QUARK_ENSURE(
      surface != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::ApiError,
                "PresenterBundle::create_surface: create_surface returned "
                "VK_NULL_HANDLE"));

  return surface;
}

void destroy_surface(VkInstance instance, VkSurfaceKHR &surface) noexcept {
  if (instance == VK_NULL_HANDLE || surface == VK_NULL_HANDLE) {
    return;
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  surface = VK_NULL_HANDLE;
}

} // namespace

util::Status PresenterBundle::create(const CreateInfo &ci) {
  destroy();

  create_info_ = ci;

  QUARK_TRY_ASSIGN(surface_, create_surface(ci.instance, ci.window));

  Swapchain::CreateInfo sci{};
  sci.physical_device = ci.physical_device;
  sci.device = ci.device;
  sci.surface = surface_;
  sci.graphics_queue_family_index = ci.graphics_queue_family_index;
  sci.present_queue_family_index = ci.present_queue_family_index;
  sci.window = ci.window;
  sci.old_swapchain = VK_NULL_HANDLE;
  sci.preferred_present_mode = ci.preferred_present_mode;
  sci.preferred_format = ci.preferred_format;
  sci.preferred_color_space = ci.preferred_color_space;

  if (auto res = swapchain_.create(sci); !res) {
    destroy_surface(ci.instance, surface_);
    return util::unexpected(std::move(res.error()));
  }

  QUARK_OK();
}

util::Status PresenterBundle::recreate_swapchain() {
  VkSwapchainKHR old_handle = swapchain_.handle();

  Swapchain::CreateInfo sci{};
  sci.physical_device = create_info_.physical_device;
  sci.device = create_info_.device;
  sci.surface = surface_;
  sci.graphics_queue_family_index = create_info_.graphics_queue_family_index;
  sci.present_queue_family_index = create_info_.present_queue_family_index;
  sci.window = create_info_.window;
  sci.old_swapchain = old_handle;
  sci.preferred_present_mode = create_info_.preferred_present_mode;
  sci.preferred_format = create_info_.preferred_format;
  sci.preferred_color_space = create_info_.preferred_color_space;

  Swapchain next_swapchain;
  QUARK_TRY_STATUS(next_swapchain.create(sci));
  swapchain_ = std::move(next_swapchain);
  QUARK_OK();
}

void PresenterBundle::destroy_swapchain() noexcept { swapchain_.reset(); }

void PresenterBundle::destroy() noexcept {
  destroy_swapchain();
  destroy_surface(create_info_.instance, surface_);
  create_info_ = CreateInfo{};
}

} // namespace quark::vk