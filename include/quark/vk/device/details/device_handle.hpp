#pragma once

#include <cstdint>

namespace quark::vk::details {

struct DeviceHandle {
  uint32_t index = 0xFFFF'FFFFU;
  uint32_t generation = 0;

  [[nodiscard]] constexpr bool valid() const noexcept {
    return index != 0xFFFF'FFFFU;
  }

  [[nodiscard]] friend constexpr bool operator==(DeviceHandle a,
                                                 DeviceHandle b) noexcept {
    return a.index == b.index && a.generation == b.generation;
  }
};

} // namespace quark::vk::details
