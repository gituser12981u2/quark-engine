#include <cstdint>
#include <cstdlib>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <quark/vk/instance/details/instance_handle.hpp>
#include <quark/vk/instance/details/instance_registry.hpp>
#include <vulkan/vulkan.h>

namespace quark::vk::details {

bool InstanceRegistry::matches_(InstanceHandle handle, const Slot &s) noexcept {
  return handle.valid() && s.live && s.generation == handle.generation;
}

void InstanceRegistry::clear() noexcept {
  for (auto &s : slots_) {
    if (s.live) {
      s.instance.destroy();
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

util::Result<InstanceHandle>
InstanceRegistry::create(const Instance::CreateInfo &ci) {
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
    s.instance.destroy();
    s.live = false;
    ++s.generation;
  }

  QUARK_TRY_STATUS(s.instance.create(ci));
  s.live = true;

  return InstanceHandle{.index = idx, .generation = s.generation};
}

void InstanceRegistry::destroy(InstanceHandle handle) noexcept {
  if (!handle.valid() || handle.index >= slots_.size()) {
    return;
  }

  Slot &s = slots_[handle.index];
  if (!matches_(handle, s)) {
    return;
  }

  s.instance.destroy();
  s.live = false;
  ++s.generation;

  free_.push_back(handle.index);
}

bool InstanceRegistry::alive(InstanceHandle handle) const noexcept {
  if (!handle.valid() || handle.index >= slots_.size()) {
    return false;
  }

  return matches_(handle, slots_[handle.index]);
}

Instance *InstanceRegistry::get(InstanceHandle handle) noexcept {
  if (!alive(handle)) {
    return nullptr;
  }

  return &slots_[handle.index].instance;
}

const Instance *InstanceRegistry::get(InstanceHandle handle) const noexcept {
  if (!alive(handle)) {
    return nullptr;
  }

  return &slots_[handle.index].instance;
}

} // namespace quark::vk::details
