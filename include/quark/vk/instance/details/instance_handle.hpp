#pragma once

#include <cstdint>

namespace quark::vk {

/**
 * @brief Opaque handle to a registry-managed InstanceBundle.
 *
 * Generation prevents use-after-free when slots are reused.
 */
struct InstanceHandle {
  uint32_t index = 0xFFFF'FFFFU;
  uint32_t generation = 0;

  [[nodiscard]] constexpr bool valid() const noexcept {
    return index != 0xFFFF'FFFFU;
  }

  [[nodiscard]] friend constexpr bool operator==(InstanceHandle a,
                                                 InstanceHandle b) noexcept {
    return a.index == b.index && a.generation == b.generation;
  }
};

} // namespace quark::vk
