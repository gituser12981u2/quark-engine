#pragma once

#include "quark/engine/handle/opaque_handle.hpp"
#include "quark/engine/registry/registry_base.hpp"
#include "quark/engine/retire/retirement_queue.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include <cstdint>
#include <quark/utils/diagnostic.hpp>
#include <utility>

namespace quark::engine {

template <class Backend> struct BackendRegistrySlot {
  bool live{false};
  uint32_t generation{1};
  Backend backend;
};

template <class Backend> struct RetiredBackendPayload {
  Backend backend;
};

template <class Backend> struct BackendRegistryPolicy {
  static void
  destroy_slot_immediate(BackendRegistrySlot<Backend> &slot) noexcept {
    slot.backend.destroy();
  }

  static void move_slot_to_retired_payload(
      BackendRegistrySlot<Backend> &slot,
      RetiredBackendPayload<Backend> &payload) noexcept {
    payload.backend = std::move(slot.backend);
  }

  static void destroy_retired_payload(void *ctx) noexcept {
    auto *payload = static_cast<RetiredBackendPayload<Backend> *>(ctx);
    payload->backend.destroy();
  }

  static void cleanup_retired_payload(void *ctx) noexcept {
    delete static_cast<RetiredBackendPayload<Backend> *>(ctx);
  }
};

template <OpaqueHandle Handle, class Backend>
class BackendRegistry final
    : private RegistryBase<Handle, BackendRegistrySlot<Backend>,
                           RetiredBackendPayload<Backend>,
                           BackendRegistryPolicy<Backend>> {
private:
  using Slot = BackendRegistrySlot<Backend>;
  using RetiredPayload = RetiredBackendPayload<Backend>;
  using Policy = BackendRegistryPolicy<Backend>;

  using Base = RegistryBase<Handle, Slot, RetiredPayload, Policy>;

public:
  BackendRegistry() = default;
  ~BackendRegistry() { clear(); }

  QUARK_MOVE_ONLY(BackendRegistry);

  template <class CreateInfo>
  util::Result<Handle> create_backend(const CreateInfo &ci) {
    auto pending = Base::begin_create_();

    QUARK_TRY_STATUS(pending.slot().backend.create(ci));

    return pending.commit();
  }

  void destroy_backend(Handle handle) noexcept {
    Base::destroy_live_slot_immediate_(handle);
  }

  void retire(Handle handle, RetirementQueue *retire_queue,
              uint64_t retire_at) noexcept {
    Base::retire_live_slot_(handle, retire_queue, retire_at);
  }

  void clear() noexcept { Base::clear_slots_immediate_(); }

  using Base::alive;

  [[nodiscard]] Backend *backend(Handle handle) noexcept {
    Slot *slot = Base::slot_if_live_(handle);

    return slot == nullptr ? nullptr : &slot->backend;
  }

  [[nodiscard]] const Backend *backend(Handle handle) const noexcept {
    const Slot *slot = Base::slot_if_live_(handle);

    return slot == nullptr ? nullptr : &slot->backend;
  }
};

} // namespace quark::engine
