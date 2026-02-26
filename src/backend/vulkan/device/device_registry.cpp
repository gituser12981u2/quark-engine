#include <cstdint>
#include <quark/utils/diagnostic.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/details/device.hpp>
#include <quark/vk/device/details/device_handle.hpp>
#include <quark/vk/device/details/device_registry.hpp>

namespace quark::vk {

bool DeviceRegistry::matches_(DeviceHandle handle, const Slot &s) noexcept {
  return handle.valid() && s.live && s.generation == handle.generation;
}

void DeviceRegistry::clear() noexcept {
  for (auto &s : slots_) {
    if (s.live) {
      s.device.destroy();
      s.live = false;
      ++s.generation;
    }
  }

  free_.clear();
  free_.reserve(slots_.size());
  for (uint32_t i = 0; i < slots_.size(); ++i) {
    free_.push_back(i);
  }
}

util::Result<DeviceHandle>
DeviceRegistry::create(const Device::CreateInfo &ci) {
  uint32_t idx = 0;

  if (!free_.empty()) {
    idx = free_.back();
    free_.pop_back();
  } else {
    idx = static_cast<uint32_t>(slots_.size());
    slots_.push_back(Slot{});
  }

  Slot &s = slots_[idx];

  if (s.live) {
    s.device.destroy();
    s.live = false;
    ++s.generation;
  }

  QUARK_TRY_STATUS(s.device.create(ci));
  s.live = true;

  return DeviceHandle{.index = idx, .generation = s.generation};
}

void DeviceRegistry::destroy(DeviceHandle handle) noexcept {
  if (!handle.valid() || handle.index >= slots_.size()) {
    return;
  }

  Slot &s = slots_[handle.index];
  if (!matches_(handle, s)) {
    return;
  }

  s.device.destroy();
  s.live = false;
  ++s.generation;

  free_.push_back(handle.index);
}

bool DeviceRegistry::alive(DeviceHandle handle) const noexcept {
  if (!handle.valid() || handle.index >= slots_.size()) {
    return false;
  }

  return matches_(handle, slots_[handle.index]);
}

Device *DeviceRegistry::get(DeviceHandle handle) noexcept {
  if (!alive(handle)) {
    return nullptr;
  }

  return &slots_[handle.index].device;
}

const Device *DeviceRegistry::get(DeviceHandle handle) const noexcept {
  if (!alive(handle)) {
    return nullptr;
  }

  return &slots_[handle.index].device;
}

} // namespace quark::vk
