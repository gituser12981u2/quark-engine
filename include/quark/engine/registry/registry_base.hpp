#pragma once

#include <cassert>
#include <cstdint>
#include <new>
#include <quark/engine/handle/opaque_handle.hpp>
#include <quark/engine/registry/registry_concepts.hpp>
#include <quark/engine/retire/retirement_queue.hpp>
#include <quark/utils/raii.hpp>
#include <utility>
#include <vector>

namespace quark::engine {

template <OpaqueHandle Handle, RegistrySlot Slot, class RetiredPayload,
          class Policy>
  requires RegistryPolicy<Policy, Slot, RetiredPayload>
class RegistryBase {
public:
  RegistryBase() = default;
  ~RegistryBase() = default;

  QUARK_MOVE_ONLY(RegistryBase);

  [[nodiscard]] bool alive(Handle handle) const noexcept {
    if (!handle.valid() || handle.index >= slots_.size()) {
      return false;
    }

    return matches_(handle, slots_[handle.index]);
  }

protected:
  class PendingSlot {
  public:
    PendingSlot(RegistryBase &registry, Handle handle) noexcept
        : registry_(&registry), handle_(handle) {}

    PendingSlot(PendingSlot &&other) noexcept
        : registry_(std::exchange(other.registry_, nullptr)),
          handle_(other.handle_),
          committed_(std::exchange(other.committed_, true)) {}

    PendingSlot &operator=(PendingSlot &&) = delete;

    PendingSlot(const PendingSlot &) = delete;
    PendingSlot &operator=(const PendingSlot &) = delete;

    ~PendingSlot() {
      if (registry_ == nullptr || committed_) {
        return;
      }

      registry_->destroy_unpublished_slot_(handle_);
      registry_->release_unpublished_handle_(handle_);
    }

    [[nodiscard]] Slot &slot() noexcept {
      return registry_->slots_[handle_.index];
    }

    [[nodiscard]] Handle handle() const noexcept { return handle_; }

    [[nodiscard]] Handle commit() noexcept {
      Slot &s = slot();
      s.live = true;
      committed_ = true;
      return handle_;
    }

  private:
    RegistryBase *registry_ = nullptr;
    Handle handle_{};
    bool committed_ = false;
  };

  [[nodiscard]] PendingSlot begin_create_() {
    return PendingSlot{*this, allocate_handle_()};
  }

  [[nodiscard]] Slot *slot_if_live_(Handle handle) noexcept {
    if (!alive(handle)) {
      return nullptr;
    }

    return &slots_[handle.index];
  }

  [[nodiscard]] const Slot *slot_if_live_(Handle handle) const noexcept {
    if (!alive(handle)) {
      return nullptr;
    }

    return &slots_[handle.index];
  }

  void retire_live_slot_(Handle handle, uint64_t retire_at) noexcept {
    if (retire_queue_ == nullptr) {
      return;
    }

    Slot *slot = slot_if_live_(handle);
    if (slot == nullptr) {
      return;
    }

    auto *payload = new (std::nothrow) RetiredPayload{};
    if (payload == nullptr) {
      Policy::destroy_slot_immediate(*slot);
      retire_slot_metadata_(handle, *slot);
      return;
    }

    Policy::move_slot_to_retired_payload(*slot, *payload);
    retire_slot_metadata_(handle, *slot);

    ::quark::vk::RetirementQueue::Task task{};
    task.fn = &Policy::destroy_retired_payload;
    task.cleanup = &Policy::cleanup_retired_payload;
    task.ctx = payload;

    auto res = retire_queue_->enqueue(retire_at, task);
    if (!res) {
      Policy::destroy_retired_payload(payload);
      Policy::cleanup_retired_payload(payload);
    }
  }

  void clear_slots_immediate_() noexcept {
    for (Slot &slot : slots_) {
      if (!slot.live) {
        continue;
      }

      Policy::destroy_slot_immediate(slot);
      slot.live = false;
      ++slot.generation;
    }

    slots_.clear();
    free_.clear();
    retire_queue_ = nullptr;
  }

  [[nodiscard]] static bool matches_(Handle handle, const Slot &slot) noexcept {
    return handle.valid() && slot.live && slot.generation == handle.generation;
  }

  // TODO: make RetirementQueue not vk specific
  vk::RetirementQueue *retire_queue_ = nullptr;
  std::vector<Slot> slots_;
  std::vector<uint32_t> free_;

private:
  [[nodiscard]] Handle allocate_handle_() {
    uint32_t index = 0;

    if (!free_.empty()) {
      index = free_.back();
      free_.pop_back();
    } else {
      index = static_cast<uint32_t>(slots_.size());
      slots_.push_back(Slot{});
    }

    Slot &slot = slots_[index];
    assert(!slot.live && "registry free-list contained a live slot");

    return Handle{.index = index, .generation = slot.generation};
  }

  void release_unpublished_handle_(Handle handle) noexcept {
    if (handle.index >= slots_.size()) {
      return;
    }

    Slot &slot = slots_[handle.index];

    if (slot.live) {
      return;
    }

    free_.push_back(handle.index);
  }

  void destroy_unpublished_slot_(Handle handle) noexcept {
    if (handle.index >= slots_.size()) {
      return;
    }

    Slot &slot = slots_[handle.index];
    if (slot.live) {
      return;
    }

    Policy::destroy_slot_immediate(slot);
  }

  void retire_slot_metadata_(Handle handle, Slot &slot) noexcept {
    slot.live = false;
    ++slot.generation;
    free_.push_back(handle.index);
  }
};

} // namespace quark::engine
