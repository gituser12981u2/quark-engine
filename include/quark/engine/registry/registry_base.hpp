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

/**
 * @brief Implementation base for typed generational resource registries.
 *
 * Implemented according to RFC-0002.
 *
 * RegistryBase provides the common mechanics for registries that address
 * resources through opaque generationl handles. It owns slot storage, free-list
 * reuse, handle validation, transactional slot creation, and deferred
 * retirement transfer.
 *
 * Registries provide resoruce-specific creation and accessors. Payload
 * destruction and retirement behavior is supplied by Policy.
 *
 * @tparam Handle Opaque generational handle type satisfying OpaqueHandle.
 * @tparam Slot Slot type satisfying RegistrySlot.
 * @tparam RetiredPayload Payload type submitted to the retirement queue.
 * @tparam Policy Lifecycle policy satisfying RegistryPolicy<Policy, Slot,
 * RetiredPayload>.
 *
 * @section registry_base_creation Creation model
 *
 * Creation is transactional. A concrete registry calls begin_create_() to
 * reserve an unpublished slot, initializes the slot payload, and publishes the
 * slot by calling PendingSlot::commit(). If creation exits before commit, the
 * pending slot is destroyed and returned to the free list.
 *
 * @section registry_base_retirement Retirement model
 *
 * retire_live_slot_() immediately invalidates the handle by marking the slot
 * dead and incrementing its generation. The payload is moved into a
 * RetiredPayload and queued for deferred destruction. If allocation or enqueue
 * fails, the payload is destroyed immediately.
 *
 * @section registry_base_invariants Invariants
 *
 * - A live slot is reachable only through a handle with matching generation.
 * - A stale handle must not resolve to a different live resource.
 * - free_ must not contain the index of a live slot.
 * - Retiring a live slot invalidates existing handles to that slot.
 * - Uncommitted creation must not publish a live handle.
 *
 * @note This class is intended as a protected implementation base for typed
 * registry facades. it is not a runtime-polymorphic interface.
 */
template <OpaqueHandle Handle, RegistrySlot Slot, class RetiredPayload,
          class Policy>
  requires RegistryPolicy<Policy, Slot, RetiredPayload>
class RegistryBase {
public:
  RegistryBase() = default;
  ~RegistryBase() = default;

  QUARK_MOVE_ONLY(RegistryBase);

  /**
   * @brief Checks whether a handle currently refers to a live slot.
   *
   * This rejects null, out-of-bounds, stale, and dead handles.
   *
   * @param handle Handle to check.
   * @return true if the handle matches a live slot; false otherwise.
   */
  [[nodiscard]] bool alive(Handle handle) const noexcept {
    if (!handle.valid() || handle.index >= slots_.size()) {
      return false;
    }

    return matches_(handle, slots_[handle.index]);
  }

protected:
  /**
   * @class PendingSlot
   * @brief RAII guard for transactional slot creation.
   *
   * A PendingSlot represents a slot reserved by begin_create_() but not yet
   * visible through alive(). Destroying an uncommitted guard rolls back the
   * reservation by destroying the slot payload and returning the index to the
   * free list.
   *
   * @note commit() must be called exactly once after the concrete registry has
   * fully initialized the slot payload.
   */
  class PendingSlot {
  public:
    /**
     * @brief Constructs a pending slot guard.
     *
     * @param registry Registry that owns the slot.
     * @param handle Handle reserved for the slot.
     */
    PendingSlot(RegistryBase &registry, Handle handle) noexcept
        : registry_(&registry), handle_(handle) {}

    /**
     * @brief Move-constructs a pending slot guard.
     *
     * Ownership of the rollback responsibility is transferred to the new guard.
     */
    PendingSlot(PendingSlot &&other) noexcept
        : registry_(std::exchange(other.registry_, nullptr)),
          handle_(other.handle_),
          committed_(std::exchange(other.committed_, true)) {}

    PendingSlot &operator=(PendingSlot &&) = delete;

    PendingSlot(const PendingSlot &) = delete;
    PendingSlot &operator=(const PendingSlot &) = delete;

    /**
     * @brief Rolls back the unpublished slot if it was not committed.
     */
    ~PendingSlot() {
      if (registry_ == nullptr || committed_) {
        return;
      }

      registry_->destroy_unpublished_slot_(handle_);
      registry_->release_unpublished_handle_(handle_);
    }

    /**
     * @brief Returns the reserved slot.
     */
    [[nodiscard]] Slot &slot() noexcept {
      return registry_->slots_[handle_.index];
    }

    /**
     * @brief Returns the handle reserved for the slot.
     */
    [[nodiscard]] Handle handle() const noexcept { return handle_; }

    /**
     * @brief Publishes the slot as live and releases rollback ownership.
     *
     * @pre The slot payload has been fully initialized.
     *
     * @post The slot is live.
     * @post alive(returned_handle) is true.
     *
     * @return Handle for the now-live slot.
     */
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

  /**
   * @brief Begins transactional creation of a registry slot.
   *
   * The returned PendingSlot owns an unpublished slot reservation. If commit()
   * is not called before the guard is destroyed, the slot is cleaned up and its
   * index is returned to the free list.
   *
   * @return Pending slot guard.
   */
  [[nodiscard]] PendingSlot begin_create_() {
    return PendingSlot{*this, allocate_handle_()};
  }

  /**
   * @brief Returns a mutable slot pointer if the handle is live.
   *
   * @param handle Handle to look up.
   * @return Pointer to the live slot, or nullptr if the handle is invalid,
   * stale, or dead.
   */
  [[nodiscard]] Slot *slot_if_live_(Handle handle) noexcept {
    if (!alive(handle)) {
      return nullptr;
    }

    return &slots_[handle.index];
  }

  /**
   * @brief Returns a const slot pointer if the handle is live.
   *
   * @param handle Handle to look up.
   * @return Pointer to the live slot, or nullptr if the handle is invalid,
   * stale, or dead.
   */
  [[nodiscard]] const Slot *slot_if_live_(Handle handle) const noexcept {
    if (!alive(handle)) {
      return nullptr;
    }

    return &slots_[handle.index];
  }

  /**
   * @brief Retires the source stored in a live slot.
   *
   * If the handle is invalid, stale, or dead, this is a no-op. if the
   * retirement queue is unavailable, this is also a no-op.
   *
   * @post If handle denoted a live slot on entry, alive(handle) is false after
   * return.
   *
   * @param handle Handle identifying the live slot to retire.
   * @param retire_queue RetirementQueue from which to retire.
   * @param retire_at Retirement timeline values.
   */
  void retire_live_slot_(Handle handle, RetirementQueue *retire_queue,
                         uint64_t retire_at) noexcept {
    Slot *slot = slot_if_live_(handle);
    if (slot == nullptr) {
      return;
    }

    if (retire_queue == nullptr) {
      Policy::destroy_slot_immediate(*slot);
      retire_slot_metadata_(handle, *slot);
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

    ::quark::engine::RetirementQueue::Task task{};
    task.fn = &Policy::destroy_retired_payload;
    task.cleanup = &Policy::cleanup_retired_payload;
    task.ctx = payload;

    auto result = retire_queue->enqueue(retire_at, task);
    if (!result) {
      Policy::destroy_retired_payload(payload);
      Policy::cleanup_retired_payload(payload);
    }
  }

  /**
   * @brief Immediately destroys all live slots and clears registry storage.
   *
   * This does not enqueue retirement tasks. It is intended for teardown paths
   * where immediate destruction is required.
   */
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
  }

  /**
   * @brief Immediately destroys one live slot.
   *
   * If the handle is invalid, stale, or dead, this is a no-op.
   *
   * This does not enqueue retirement work. The slot is invalidated immediately,
   * its generation is incremented, and its index is returned to the free list.
   *
   * @param handle Handle identifying the live slot to destroy.
   */
  void destroy_live_slot_immediate_(Handle handle) noexcept {
    Slot *slot = slot_if_live_(handle);
    if (slot == nullptr) {
      return;
    }

    Policy::destroy_slot_immediate(*slot);
    retire_slot_metadata_(handle, *slot);
  }

  /**
   * @brief Checks whether a handle matches a specific live slot.
   *
   * @param handle Handle to compare.
   * @param slot Slot to compare again.
   * @return if the handle is non-null, the slot is live, and the generation
   * values match.
   */
  [[nodiscard]] static bool matches_(Handle handle, const Slot &slot) noexcept {
    return handle.valid() && slot.live && slot.generation == handle.generation;
  }

  /// Slot storage owned by the registry.
  std::vector<Slot> slots_;

  /// Indices of non-live slots available for reuse.
  std::vector<uint32_t> free_;

private:
  /**
   * @brief Allocates or reuses a slot and returns a handle for it.
   *
   * The returned handle refers to an unpublished slot. The slot is not live
   * until the corresponding PendingSlot is committed.
   *
   * @return Handle for the reserved slot.
   */
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

  /**
   * @brief Releases an unpublished handle reservation.
   *
   * This returns the handle's slot index to the free list only if the slot is
   * still unpublished.
   *
   * @param handle Handle reserved for the unpublished slot.
   */
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

  /**
   * @brief Destroys any payload currently stores in an unpublished slot.
   *
   * This is used by PendingSlot rollback. It assumes that the slot is not live,
   * but may contain partially constructed resource state from a failed create
   * path.
   *
   * @param handle Handle reserved for the unpublished slot.
   */
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

  /**
   * @brief Marks a live slot as dead and makes its index reusable.
   *
   * This increments the slot generation so existing handles become stale.
   *
   * @param handle Handle identifying the slot.
   * @param slot Slot being retired.
   */
  void retire_slot_metadata_(Handle handle, Slot &slot) noexcept {
    slot.live = false;
    ++slot.generation;
    free_.push_back(handle.index);
  }
};

} // namespace quark::engine
