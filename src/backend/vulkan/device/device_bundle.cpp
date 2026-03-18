#include <cstdint>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/device/details/device.hpp>
#include <quark/vk/device/details/device_handle.hpp>
#include <quark/vk/device/details/device_registry.hpp>
#include <quark/vk/device/device_bundle.hpp>
#include <quark/vk/device/device_view.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status DeviceBundle::create(const DeviceBundle::CreateInfo &ci) {
  destroy();

  QUARK_TRY_ASSIGN(handle_, registry_.create(ci.device));
  QUARK_OK();
}

void DeviceBundle::destroy() noexcept {
  registry_.destroy(handle_);
  handle_ = details::DeviceHandle{};
}

util::Status DeviceBundle::validate() const noexcept {
  QUARK_ENSURE(registry_.alive(handle_),
               QUARK_ERR(util::Errc::InvalidState, "device handle not alive"));

  return ::quark::vk::validate(view());
}

DeviceView DeviceBundle::view() const noexcept {
  const details::Device *dev = registry_.get(handle_);
  if (dev == nullptr) {
    return {};
  }

  DeviceView v{};
  v.physical_device = dev->vk_physical_device();
  v.device = dev->vk_device();
  v.graphics_queue = dev->graphics_queue();
  v.graphics_queue_family_index = dev->graphics_queue_family_index();

  return v;
}

VkPhysicalDevice DeviceBundle::vk_physical_device() const noexcept {
  const details::Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->vk_physical_device() : VK_NULL_HANDLE;
}

VkDevice DeviceBundle::vk_device() const noexcept {
  const details::Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->vk_device() : VK_NULL_HANDLE;
}

VkQueue DeviceBundle::graphics_queue() const noexcept {
  const details::Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->graphics_queue() : VK_NULL_HANDLE;
}

VkQueue DeviceBundle::present_queue() const noexcept {
  const details::Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->present_queue() : VK_NULL_HANDLE;
}

uint32_t DeviceBundle::graphics_queue_family_index() const noexcept {
  const details::Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->graphics_queue_family_index() : 0;
}

uint32_t DeviceBundle::present_queue_family_index() const noexcept {
  const details::Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->present_queue_family_index() : 0;
}

details::Device::Capabilities DeviceBundle::capabilities() const noexcept {
  const details::Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->capabilities()
                             : details::Device::Capabilities{};
}

} // namespace quark::vk
