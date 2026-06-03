#include <quark/vk/device/device_view.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/frame/details/frame_cmd.hpp>
#include <quark/vk/frame/details/frame_handle.hpp>
#include <quark/vk/frame/details/frame_registry.hpp>
#include <quark/vk/frame/details/frame_sync.hpp>

#include <cstdint>
#include <cstdlib>
#include <utility>

namespace quark::vk::details {

void FrameRegistryPolicy::destroy_slot_immediate(
    FrameRegistrySlot &slot) noexcept {
  slot.cmd.destroy();
  slot.sync.destroy();
}

void FrameRegistryPolicy::move_slot_to_retired_payload(
    FrameRegistrySlot &slot, FrameRetiredPayload &payload) noexcept {
  payload.cmd = std::move(slot.cmd);
  payload.sync = std::move(slot.sync);
}

void FrameRegistryPolicy::destroy_retired_payload(void *ctx) noexcept {
  auto *payload = static_cast<FrameRetiredPayload *>(ctx);
  if (payload == nullptr) {
    return;
  }

  payload->cmd.destroy();
  payload->sync.destroy();
}

void FrameRegistryPolicy::cleanup_retired_payload(void *ctx) noexcept {
  auto *payload = static_cast<FrameRetiredPayload *>(ctx);
  delete payload;
}

util::Result<FrameHandle> FrameRegistry::create(const CreateInfo &ci) {
  QUARK_TRY_VALIDATE(validate(ci.device));

  QUARK_ENSURE(ci.retire_queue != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "retire_queue is null"));

  QUARK_ENSURE(
      ci.frames_in_flight > 0,
      QUARK_ERR(util::Errc::InvalidArg, "frames_in_flight must be > 0"));

  retire_queue_ = ci.retire_queue;

  auto pending = begin_create_();
  FrameRegistrySlot &slot = pending.slot();

  FrameCmd::CreateInfo cmd_ci{};
  cmd_ci.device = ci.device;
  cmd_ci.buffer_count = ci.cmd_buffer_count;
  cmd_ci.pool_flags = ci.cmd_pool_flags;
  cmd_ci.allocator = ci.allocator;

  FrameSync::CreateInfo sync_ci{};
  sync_ci.device = ci.device;
  sync_ci.frames_in_flight = ci.frames_in_flight;
  sync_ci.allocator = ci.allocator;
  sync_ci.frames_idle_on_create = ci.frames_idle_on_create;

  QUARK_TRY_STATUS(slot.cmd.create(cmd_ci));
  QUARK_TRY_STATUS(slot.sync.create(sync_ci));

  return pending.commit();
}

void FrameRegistry::destroy(FrameHandle handle, uint64_t retire_at) noexcept {
  retire_live_slot_(handle, retire_at);
}

void FrameRegistry::clear() noexcept { clear_slots_immediate_(); }

FrameCmd *FrameRegistry::cmd(FrameHandle handle) noexcept {
  FrameRegistrySlot *slot = slot_if_live_(handle);
  if (slot == nullptr) {
    return nullptr;
  }

  return &slot->cmd;
}

const FrameCmd *FrameRegistry::cmd(FrameHandle handle) const noexcept {
  const FrameRegistrySlot *slot = slot_if_live_(handle);
  if (slot == nullptr) {
    return nullptr;
  }

  return &slot->cmd;
}

FrameSync *FrameRegistry::sync(FrameHandle handle) noexcept {
  FrameRegistrySlot *slot = slot_if_live_(handle);
  if (slot == nullptr) {
    return nullptr;
  }

  return &slot->sync;
}

const FrameSync *FrameRegistry::sync(FrameHandle handle) const noexcept {
  const FrameRegistrySlot *slot = slot_if_live_(handle);
  if (slot == nullptr) {
    return nullptr;
  }

  return &slot->sync;
}

} // namespace quark::vk::details
