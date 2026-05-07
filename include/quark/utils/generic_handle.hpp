#pragma once

#include <concepts>
#include <cstdint>
#include <limits>

namespace util {

template <class H>
concept OpaqueHandle = std::equality_comparable<H> && requires(H handle) {
  { handle.index } -> std::convertible_to<uint32_t>;
  { handle.generation } -> std::convertible_to<uint32_t>;
};

template <class Tag> struct GenericHandle final {
  static constexpr uint32_t invalid_index =
      std::numeric_limits<uint32_t>::max();

  uint32_t index = invalid_index;
  uint32_t generation = 0;

  [[nodiscard]] constexpr bool valid() const noexcept {
    return index != invalid_index;
  }

  [[nodiscard]] friend constexpr bool
  operator==(GenericHandle, GenericHandle) noexcept = default;
};

static_assert(OpaqueHandle<GenericHandle<void>>);

} // namespace util