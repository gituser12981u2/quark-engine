#include <cstdint>
#include <cstdlib>
#include <new>
#include <quark/vk/device/device_view.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/frame/details/frame_cmd.hpp>
#include <quark/vk/frame/details/frame_handle.hpp>
#include <quark/vk/frame/details/frame_registry.hpp>
#include <quark/vk/frame/details/frame_sync.hpp>
#include <utility>

namespace quark::vk::details {

bool FrameRegistry::matches_(FrameHandle handle, const Slot &slot) noexcept {
  return handle.valid() && slot.live && slot.generation == handle.generation;
}

void FrameRegistry::destroy_retired_frame_(void *ctx) noexcept {
  auto *payload = static_cast<RetiredFramePayload *>(ctx);
  if (payload == nullptr) {
    return;
  }

  payload->cmd.destroy();
  payload->sync.destroy();
}

void FrameRegistry::cleanup_retired_frame_(void *ctx) noexcept {
  auto *payload = static_cast<RetiredFramePayload *>(ctx);
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

  uint32_t idx = 0;
  if (!free_.empty()) {
    idx = free_.back();
    free_.pop_back();
  } else {
    idx = static_cast<uint32_t>(slots_.size());
    slots_.push_back(Slot{});
  }

  Slot &slot = slots_[idx];

  if (slot.live) {
    QUARK_LOG_WARN("Slot reused");
    slot.cmd.destroy();
    slot.sync.destroy();
    slot.live = false;
    ++slot.generation;
  }

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

  slot.live = true;

  return FrameHandle{.index = idx, .generation = slot.generation};
}

void FrameRegistry::destory(FrameHandle handle, uint64_t retire_at) noexcept {
  if (retire_queue_ == nullptr) {
    return;
  }

  if (!handle.valid() || handle.index >= slots_.size()) {
    return;
  }

  Slot &slot = slots_[handle.index];
  if (!matches_(handle, slot)) {
    return;
  }

  auto *payload = new (std::nothrow) RetiredFramePayload{};
  if (payload == nullptr) {
    QUARK_LOG_WARN("Payload is erroneously nullptr, falling back");
    slot.cmd.destroy();
    slot.sync.destroy();
    slot.live = false;
    ++slot.generation;
    free_.push_back(handle.index);
    return;
  }

  payload->cmd = std::move(slot.cmd);
  payload->sync = std::move(slot.sync);

  slot.live = false;
  ++slot.generation;
  free_.push_back(handle.index);

  RetirementQueue::Task task{};
  task.fn = &FrameRegistry::destroy_retired_frame_;
  task.cleanup = &FrameRegistry::cleanup_retired_frame_;
  task.ctx = payload;

  auto res = retire_queue_->enqueue(retire_at, task);
  if (!res) {
    destroy_retired_frame_(payload);
    cleanup_retired_frame_(payload);
  }
}

void FrameRegistry::clear() noexcept {
  for (auto &slot : slots_) {
    if (slot.live) {
      slot.cmd.destroy();
      slot.sync.destroy();
      slot.live = false;
      ++slot.generation;
    }
  }

  slots_.clear();
  free_.clear();
  retire_queue_ = nullptr;
}

bool FrameRegistry::alive(FrameHandle handle) const noexcept {
  if (!handle.valid() || handle.index >= slots_.size()) {
    return false;
  }

  return matches_(handle, slots_[handle.index]);
}

FrameCmd *FrameRegistry::cmd(FrameHandle handle) noexcept {
  if (!alive(handle)) {
    return nullptr;
  }

  return &slots_[handle.index].cmd;
}

const FrameCmd *FrameRegistry::cmd(FrameHandle handle) const noexcept {
  if (!alive(handle)) {
    return nullptr;
  }

  return &slots_[handle.index].cmd;
}

FrameSync *FrameRegistry::sync(FrameHandle handle) noexcept {
  if (!alive(handle)) {
    return nullptr;
  }

  return &slots_[handle.index].sync;
}

const FrameSync *FrameRegistry::sync(FrameHandle handle) const noexcept {
  if (!alive(handle)) {
    return nullptr;
  }

  return &slots_[handle.index].sync;
}

} // namespace quark::vk::details
