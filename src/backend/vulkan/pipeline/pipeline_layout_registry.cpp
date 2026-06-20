#include "quark/vk/pipeline/details/pipeline_layout_registry.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/descriptor/details/descriptor_set_layout_handle.hpp"
#include "quark/vk/device/device_view.hpp"
#include "quark/vk/pipeline/details/pipeline_layout.hpp"
#include "quark/vk/pipeline/details/pipeline_layout_handle.hpp"
#include "quark/vk/pipeline/pipeline_limits.hpp"
#include <array>
#include <cstdint>
#include <utility>

namespace quark::vk::details {

void PipelineLayoutRegistryPolicy::destroy_slot_immediate(
    PipelineLayoutSlot &slot) noexcept {
  slot.layout.destroy();
}

void PipelineLayoutRegistryPolicy::move_slot_to_retired_payload(
    PipelineLayoutSlot &slot, RetiredPipelineLayout &payload) noexcept {
  payload.layout = std::move(slot.layout);
}

void PipelineLayoutRegistryPolicy::destroy_retired_payload(void *ctx) noexcept {
  auto *payload = static_cast<RetiredPipelineLayout *>(ctx);
  payload->layout.destroy();
}

void PipelineLayoutRegistryPolicy::cleanup_retired_payload(void *ctx) noexcept {
  delete static_cast<RetiredPipelineLayout *>(ctx);
}

util::Status PipelineLayoutRegistry::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate(ci.device));

  destroy();

  device_ = ci.device;
  descriptor_set_layouts_ = ci.descriptor_set_layouts;
  allocator_ = ci.allocator;
  retire_queue_ = ci.retire_queue;

  QUARK_OK();
}

void PipelineLayoutRegistry::destroy() noexcept {
  clear_slots_immediate_();

  device_ = {};
  descriptor_set_layouts_ = nullptr;
  allocator_ = nullptr;
}

util::Result<PipelineLayoutHandle>
PipelineLayoutRegistry::create_layout(const LayoutCreateInfo &ci) {
  QUARK_TRY_STATUS(validate(device_));

  QUARK_ENSURE(
      ci.set_layouts.size() <= pipeline_limits::kMaxDescriptorSetLayouts,
      QUARK_ERR(util::Errc::InvalidArg,
                "pipeline layout has too many descriptor set layouts"));

  QUARK_ENSURE(ci.push_constant_ranges.size() <=
                   pipeline_limits::kMaxPushConstantRanges,
               QUARK_ERR(util::Errc::InvalidArg,
                         "pipeline layout has too many push constant ranges"));

  if (!ci.set_layouts.empty()) {
    QUARK_ENSURE(descriptor_set_layouts_ != nullptr,
                 QUARK_ERR(util::Errc::InvalidArg,
                           "pipeline layout has descriptor set layouts but no "
                           "descriptor set layout registry"));
  }

  std::array<VkDescriptorSetLayout, pipeline_limits::kMaxDescriptorSetLayouts>
      vk_set_layouts{};

  uint32_t set_layout_count = 0;
  for (DescriptorSetLayoutHandle handle : ci.set_layouts) {
    QUARK_ENSURE(
        descriptor_set_layouts_->alive(handle),
        QUARK_ERR(util::Errc::InvalidArg,
                  "pipeline layout references invalid descriptor set layout"));

    vk_set_layouts[set_layout_count++] =
        descriptor_set_layouts_->layout(handle);
  }

  auto pending = begin_create_();

  QUARK_TRY_STATUS(pending.slot().layout.create(
      {.device = device_,
       .set_layout_count = set_layout_count,
       .set_layouts = vk_set_layouts.data(),
       .push_constant_range_count =
           static_cast<uint32_t>(ci.push_constant_ranges.size()),
       .push_constant_ranges = ci.push_constant_ranges.data(),
       .allocator = allocator_}));

  return pending.commit();
}

void PipelineLayoutRegistry::clear() noexcept { clear_slots_immediate_(); }

void PipelineLayoutRegistry::destroy(PipelineLayoutHandle handle,
                                     uint64_t retire_at) noexcept {
  retire_live_slot_(handle, retire_at);
}

VkPipelineLayout
PipelineLayoutRegistry::layout(PipelineLayoutHandle handle) const noexcept {
  const PipelineLayoutSlot *slot = slot_if_live_(handle);
  if (slot == nullptr) {
    return VK_NULL_HANDLE;
  }

  return slot->layout.handle();
}

const PipelineLayout *
PipelineLayoutRegistry::backend(PipelineLayoutHandle handle) const noexcept {
  const PipelineLayoutSlot *slot = slot_if_live_(handle);
  return slot == nullptr ? nullptr : &slot->layout;
}

} // namespace quark::vk::details
