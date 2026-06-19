#include "quark/vk/descriptor/details/descriptor_set_layout_registry.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/descriptor/details/descriptor_set_layout_handle.hpp"
#include "quark/vk/device/device_view.hpp"
#include <cstdint>
#include <utility>

namespace quark::vk::details {

void DescriptorSetLayoutRegistryPolicy::destroy_slot_immediate(
    DescriptorSetLayoutSlot &slot) noexcept {
  slot.layout.destroy();
}

void DescriptorSetLayoutRegistryPolicy::move_slot_to_retired_payload(
    DescriptorSetLayoutSlot &slot,
    RetiredDescriptorSetLayout &payload) noexcept {
  payload.layout = std::move(slot.layout);
}

void DescriptorSetLayoutRegistryPolicy::destroy_retired_payload(
    void *ctx) noexcept {
  auto *payload = static_cast<RetiredDescriptorSetLayout *>(ctx);
  payload->layout.destroy();
}

void DescriptorSetLayoutRegistryPolicy::cleanup_retired_payload(
    void *ctx) noexcept {
  delete static_cast<RetiredDescriptorSetLayout *>(ctx);
}

util::Status DescriptorSetLayoutRegistry::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate(ci.device));

  destroy();

  device_ = ci.device;
  allocator_ = ci.allocator;
  retire_queue_ = ci.retire_queue;

  QUARK_OK();
}

void DescriptorSetLayoutRegistry::destroy() noexcept {
  clear_slots_immediate_();

  device_ = {};
  allocator_ = nullptr;
}

util::Result<DescriptorSetLayoutHandle>
DescriptorSetLayoutRegistry::create_layout(const LayoutCreateInfo &ci) {
  QUARK_TRY_STATUS(validate(device_));

  auto pending = begin_create_();

  QUARK_TRY_STATUS(pending.slot().layout.create(
      {.device = device_, .desc = ci.desc, .allocator = allocator_}));

  return pending.commit();
}

void DescriptorSetLayoutRegistry::clear() noexcept { clear_slots_immediate_(); }

void DescriptorSetLayoutRegistry::destroy(DescriptorSetLayoutHandle handle,
                                          uint64_t retire_at) noexcept {
  retire_live_slot_(handle, retire_at);
}

VkDescriptorSetLayout DescriptorSetLayoutRegistry::layout(
    DescriptorSetLayoutHandle handle) const noexcept {
  const DescriptorSetLayoutSlot *slot = slot_if_live_(handle);
  if (slot == nullptr) {
    return VK_NULL_HANDLE;
  }

  return slot->layout.handle();
}

} // namespace quark::vk::details
