#pragma once

#include <cstdint>
namespace quark::vk::details {

struct FrameHandle {
  uint32_t index = 0;
  uint32_t generation = 0;

  [[nodiscard]] bool valid() const noexcept { return generation != 0; }
};

} // namespace quark::vk::details
