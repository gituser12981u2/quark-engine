#pragma once

#include <concepts>
#include <cstdint>

namespace quark::engine {

/**
 * @brief Strongly typed opaque generational handle.
 *
 * Implemented according to RFC-0001.
 *
 * GenericHandle is a lightweight value type used to refer to objects stored in
 * registry-like ownership domains. The Tag parameter gives each handle family a
 * distincy type, preventing accidental use of one registry's handles with
 * another registry.
 *
 * @tparam Tag Empty tag type identifying the handle domain.
 */
template <class Tag> struct GenericHandle final {
  /// Index into the owning registry's slot storage.
  uint32_t index = 0;

  /// Generation associated with the slot at the time the handle was issued.
  uint32_t generation = 0;

  /**
   * @brief Returns whether this handle is non-null.
   *
   * @return This only checks the local sentinel state. It does not prove that
   * the handle is alive in any registry.
   */
  [[nodiscard]] constexpr bool valid() const noexcept {
    return generation != 0;
  }

  friend constexpr bool operator==(GenericHandle,
                                   GenericHandle) noexcept = default;
};

/**
 * @brief Concept for opaque generational handles.
 *
 * An opaque handle must expose an index and generation, and must support value
 * equality. Liveness is checked by the owning registry instead of the handle.
 *
 * @tparam H Handle type to check.
 */
template <class H>
concept OpaqueHandle = std::equality_comparable<H> && requires(H handle) {
  { handle.index } -> std::convertible_to<uint32_t>;
  { handle.generation } -> std::convertible_to<uint32_t>;
};

} // namespace quark::engine
