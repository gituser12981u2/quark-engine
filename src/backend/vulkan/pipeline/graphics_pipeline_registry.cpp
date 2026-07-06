#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"

#include "quark/vk/device/device_view.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_handle.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_registry.hpp"
#include "quark/vk/pipeline/details/pipeline.hpp"
#include "quark/vk/pipeline/graphics_pipeline_desc.hpp"

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

void GraphicsPipelineRegistryPolicy::destroy_slot_immediate(
    GraphicsPipelineSlot &slot) noexcept {
  slot.pipeline.destroy();
  slot.layout = {};
}

void GraphicsPipelineRegistryPolicy::move_slot_to_retired_payload(
    GraphicsPipelineSlot &slot, RetiredGraphicsPipeline &payload) noexcept {
  payload.pipeline = std::move(slot.pipeline);
  slot.layout = {};
}

void GraphicsPipelineRegistryPolicy::destroy_retired_payload(
    void *ctx) noexcept {
  auto *payload = static_cast<RetiredGraphicsPipeline *>(ctx);
  if (payload == nullptr) {
    return;
  }

  payload->pipeline.destroy();
}

void GraphicsPipelineRegistryPolicy::cleanup_retired_payload(
    void *ctx) noexcept {
  delete static_cast<RetiredGraphicsPipeline *>(ctx);
}

[[nodiscard]] util::Status
GraphicsPipelineRegistry::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate(ci.device));

  QUARK_ENSURE(ci.shaders != nullptr,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline registry shader registry is null"));

  destroy();

  device_ = ci.device;
  shaders_ = ci.shaders;
  allocator_ = ci.allocator;
  retire_queue_ = ci.retire_queue;

  QUARK_OK();
}

void GraphicsPipelineRegistry::destroy() noexcept {
  clear();

  device_ = {};
  shaders_ = nullptr;
  allocator_ = nullptr;
}

void GraphicsPipelineRegistry::clear() noexcept { clear_slots_immediate_(); }

util::Result<GraphicsPipelineHandle>
GraphicsPipelineRegistry::create_pipeline(const PipelineCreateInfo &ci) {
  QUARK_ENSURE(ci.desc != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                             "graphics pipeline desc is null"));

  QUARK_ENSURE(ci.desc->pipeline_layout->valid(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline layout view is invalid"));

  auto pending = begin_create_();

  QUARK_TRY_STATUS(pending.slot().pipeline.create_graphics({
      .device = device_,
      .extent = ci.extent,
      .desc = ci.desc,
      .shaders = shaders_,
      .pipeline_layout = ci.desc->pipeline_layout,
      .cache = VK_NULL_HANDLE,
      .allocator = allocator_,
  }));

  pending.slot().layout = ci.desc->pipeline_layout;

  return pending.commit();
}

void GraphicsPipelineRegistry::destroy(GraphicsPipelineHandle handle,
                                       uint64_t retire_at) noexcept {
  retire_live_slot_(handle, retire_at);
}

const Pipeline *GraphicsPipelineRegistry::pipeline(
    GraphicsPipelineHandle handle) const noexcept {
  const GraphicsPipelineSlot *slot = slot_if_live_(handle);
  return slot == nullptr ? nullptr : &slot->pipeline;
}

const rhi::PipelineLayout *
GraphicsPipelineRegistry::layout(GraphicsPipelineHandle handle) const noexcept {
  const GraphicsPipelineSlot *slot = slot_if_live_(handle);
  return slot == nullptr ? nullptr : slot->layout;
}

} // namespace quark::vk::details
