#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace quark::engine {

template <class T>
concept MoveOnlyNoexcept =
    std::movable<T> && !std::copy_constructible<T> &&
    !std::is_copy_assignable_v<T> && std::is_nothrow_copy_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T>;

template <class S>
concept RegistrySlot = requires(S slot) {
  { slot.live } -> std::convertible_to<bool>;
  { slot.generation } -> std::convertible_to<uint32_t>;
};

template <class P, class Slot, class RetiredPayload>
concept RegistryPolicy =
    requires(Slot &slot, RetiredPayload &payload, void *ctx) {
      { P::destroy_slot_immediate(slot) } noexcept -> std::same_as<void>;

      {
        P::move_slot_to_retired_payload(slot, payload)
      } noexcept -> std::same_as<void>;

      { P::destroy_retired_payload(ctx) } noexcept -> std::same_as<void>;
      { P::cleanup_retired_payload(ctx) } noexcept -> std::same_as<void>;
    };

} // namespace quark::engine
