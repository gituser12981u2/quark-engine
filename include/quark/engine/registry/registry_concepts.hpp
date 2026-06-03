#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace quark::engine {

/**
 * @brief Concept for move-only types with noexcept move operations.
 *
 * Implemented according to RFC-0002.
 *
 * This is for ownership-domain objects such as registries and bundles. Such
 * objects should be movable by not copyable.
 *
 * @tparam T Type to check.
 */
template <class T>
concept MoveOnlyNoexcept =
    std::movable<T> && !std::copy_constructible<T> &&
    !std::is_copy_assignable_v<T> && std::is_nothrow_move_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T>;

/**
 * @brief Concept for a registry slot.
 *
 * Implemented according to RFC-0002.
 *
 * A registry slot must track whether it currently contains a live resource and
 * the generation associated with that slot.
 *
 * @tparam S Slot type to check.
 */
template <class S>
concept RegistrySlot = requires(S slot) {
  { slot.live } -> std::convertible_to<bool>;
  { slot.generation } -> std::convertible_to<uint32_t>;
};

/**
 * @brief Concept for registry payload lifecycle policy types.
 *
 * Implemented according to RFC-0002.
 *
 * RegistryBase owns generic slot mechanics. The policy owns resource-specific
 * payload operations: immediate destruction, moving live slot data into a
 * retired payload, and final retired-payload destruction/cleanup.
 *
 * @tparam P Policy type to check.
 * @tparam Slot Slot type managed by the registry.
 * @tparam RetiredPayload Payload type queued into the retirement system.
 */
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
