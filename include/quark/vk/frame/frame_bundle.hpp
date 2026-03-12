#pragma once

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/frame/details/frame_handle.hpp>
#include <quark/vk/frame/details/frame_registry.hpp>
#include <quark/vk/frame/frame_view.hpp>

namespace quark::vk {

namespace details {

class FrameSync;
class FrameCmd;

} // namespace details

class FrameBundle final {
public:
  using CreateInfo = details::FrameRegistry::CreateInfo;

  FrameBundle() = default;
  ~FrameBundle() { destroy(); }

  QUARK_MOVE_ONLY(FrameBundle);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return registry_.alive(handle_); }

  [[nodiscard]] FrameView view() noexcept;
  [[nodiscard]] ConstFrameView view() const noexcept;

  [[nodiscard]] details::FrameCmd *cmd() noexcept;
  [[nodiscard]] const details::FrameCmd *cmd() const noexcept;
  [[nodiscard]] details::FrameSync *sync() noexcept;
  [[nodiscard]] const details::FrameSync *sync() const noexcept;

  /**
   * @brief Retires the current frame set after the given timeline value.
   *
   * After this call, the bundle becomes invalid immediately on the CPU side.
   */
  void retire(uint64_t retire_at) noexcept;

private:
  details::FrameRegistry registry_;
  details::FrameHandle handle_{};
};

} // namespace quark::vk
