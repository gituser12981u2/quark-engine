#pragma once

#include <concepts>
#include <cstdint>

namespace quark::engine {

template <class Tag> struct GenericHandle final {
  uint32_t index = 0;
  uint32_t generation = 0;

  [[nodiscard]] constexpr bool valid() const noexcept {
    return generation != 0;
  }

  friend constexpr bool operator==(GenericHandle,
                                   GenericHandle) noexcept = default;
};

template <class H>
concept OpaqueHandle = std::equality_comparable<H> && requires(H handle) {
  { handle.index } -> std::convertible_to<uint32_t>;
  { handle.generation } -> std::convertible_to<uint32_t>;
};

} // namespace quark::engine
