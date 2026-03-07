#pragma once

#include <quark/vk/frame/details/frame_handle.hpp>

namespace quark::vk {

namespace details {

class FrameCmd;
class FrameSync;

} // namespace details

struct FrameView {
  details::FrameHandle handle{};

  details::FrameCmd *cmd = nullptr;
  details::FrameSync *sync = nullptr;

  [[nodiscard]] bool valid() const noexcept {
    return handle.valid() && cmd != nullptr && sync != nullptr;
  }
};

struct ConstFrameView {
  details::FrameHandle handle{};

  const details::FrameCmd *cmd = nullptr;
  const details::FrameSync *sync = nullptr;

  [[nodiscard]] bool valid() const noexcept {
    return handle.valid() && cmd != nullptr && sync != nullptr;
  }
};

} // namespace quark::vk
