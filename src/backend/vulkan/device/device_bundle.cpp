#include <quark/vk/device/details/device.hpp>
#include <quark/vk/device/details/device_handle.hpp>
#include <quark/vk/device/details/device_registry.hpp>
#include <quark/vk/device/device_bundle.hpp>

namespace quark::vk {

DeviceBundle::DeviceBundle(const Device::CreateInfo &ci) { create(ci); }

void DeviceBundle::create(const Device::CreateInfo &ci) {
  destroy();
  handle_ = registry_.create(ci);
}

void DeviceBundle::destroy() noexcept {
  registry_.destroy(handle_);
  handle_ = DeviceHandle{};
}

VkPhysicalDevice DeviceBundle::vk_physical_device() const noexcept {
  const Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->vk_physical_device() : VK_NULL_HANDLE;
}

VkDevice DeviceBundle::vk_device() const noexcept {
  const Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->vk_device() : VK_NULL_HANDLE;
}

VkQueue DeviceBundle::graphics_queue() const noexcept {
  const Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->graphics_queue() : VK_NULL_HANDLE;
}

VkQueue DeviceBundle::present_queue() const noexcept {
  const Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->present_queue() : VK_NULL_HANDLE;
}

uint32_t DeviceBundle::graphics_queue_family_index() const noexcept {
  const Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->graphics_queue_family_index() : 0;
}

uint32_t DeviceBundle::present_queue_family_index() const noexcept {
  const Device *device = registry_.get(handle_);
  return (device != nullptr) ? device->present_queue_family_index() : 0;
}

} // namespace quark::vk
