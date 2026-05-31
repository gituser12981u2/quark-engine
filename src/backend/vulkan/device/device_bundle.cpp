#include <cstdint>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/device/details/device.hpp>
#include <quark/vk/device/device_bundle.hpp>
#include <quark/vk/device/device_view.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status DeviceBundle::create(const DeviceBundle::CreateInfo &ci) {
  destroy();

  return device_.create(ci.device);
}

void DeviceBundle::destroy() noexcept { device_.destroy(); }

util::Status DeviceBundle::validate() const noexcept {
  QUARK_ENSURE(device_.valid(),
               QUARK_ERR(util::Errc::InvalidState, "device handle not alive"));

  return ::quark::vk::validate(view());
}

DeviceView DeviceBundle::view() const noexcept {
  if (!device_.valid()) {
    return {};
  }

  DeviceView v{};
  v.physical_device = device_.vk_physical_device();
  v.device = device_.vk_device();
  v.graphics_queue = device_.graphics_queue();
  v.graphics_queue_family_index = device_.graphics_queue_family_index();

  return v;
}

VkPhysicalDevice DeviceBundle::vk_physical_device() const noexcept {
  return device_.vk_physical_device();
}

VkDevice DeviceBundle::vk_device() const noexcept {
  return device_.vk_device();
}

VkQueue DeviceBundle::graphics_queue() const noexcept {
  return device_.graphics_queue();
}

VkQueue DeviceBundle::present_queue() const noexcept {
  return device_.present_queue();
}

uint32_t DeviceBundle::graphics_queue_family_index() const noexcept {
  return device_.graphics_queue_family_index();
}

uint32_t DeviceBundle::present_queue_family_index() const noexcept {
  return device_.graphics_queue_family_index();
}

details::DeviceCapabilities DeviceBundle::capabilities() const noexcept {
  return device_.capabilities();
}

} // namespace quark::vk
