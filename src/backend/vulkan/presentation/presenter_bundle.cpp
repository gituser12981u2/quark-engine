#include <quark/platform/window/interface_query.hpp>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/presentation/presenter_bundle.hpp>
#include <quark/vk/surface_source.hpp>

#include <exception>

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

  auto swapchain_res = create_swapchain_(VK_NULL_HANDLE);
  if (!swapchain_res) {
    destroy_surface(ci.instance, surface_);
    return util::unexpected(std::move(swapchain_res.error()));
  }

  swapchain_ = std::move(swapchain_res.value());
  QUARK_OK();
}

util::Result<Swapchain>
PresenterBundle::create_swapchain_(VkSwapchainKHR old_swapchain) const {
  QUARK_ENSURE(
      surface_ != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidState, "presenter surface is null"));

  Swapchain swapchain;
  Swapchain::CreateInfo ci{};
  ci.physical_device = create_info_.physical_device;
  ci.device = create_info_.device;
  ci.surface = surface_;
  ci.graphics_queue_family_index = create_info_.graphics_queue_family_index;
  ci.present_queue_family_index = create_info_.present_queue_family_index;
  ci.window = create_info_.window;
  ci.old_swapchain = old_swapchain;
  ci.preferred_present_mode = create_info_.preferred_present_mode;
  ci.preferred_format = create_info_.preferred_format;
  ci.preferred_color_space = create_info_.preferred_color_space;

  try {
    swapchain.create(ci);
  } catch (const std::exception &e) {
    QUARK_FAIL(QUARK_ERR(util::Errc::ApiError,
                         "PresenterBundle::create_swapchain_ failed: {}",
                         e.what()));
  }

  return swapchain;
}

util::Status PresenterBundle::recreate_swapchain() {
  auto *old_swapchain = swapchain_.handle();

  Swapchain next_swapchain;
  QUARK_TRY_ASSIGN(next_swapchain, create_swapchain_(old_swapchain));

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