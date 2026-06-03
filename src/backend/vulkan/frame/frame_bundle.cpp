#include <quark/utils/diagnostic.hpp>
#include <quark/vk/frame/details/frame_cmd.hpp>
#include <quark/vk/frame/details/frame_handle.hpp>
#include <quark/vk/frame/details/frame_sync.hpp>
#include <quark/vk/frame/frame_bundle.hpp>
#include <quark/vk/frame/frame_view.hpp>

namespace quark::vk {

util::Status FrameBundle::create(const CreateInfo &ci) {
  destroy();

  QUARK_TRY_ASSIGN(handle_, registry_.create(ci));
  QUARK_OK();
}

void FrameBundle::destroy() noexcept {
  if (!handle_.valid()) {
    return;
  }

  registry_.clear();
  handle_ = details::FrameHandle{};
}

FrameView FrameBundle::view() noexcept {
  FrameView v{};
  v.handle = handle_;
  v.cmd = registry_.cmd(handle_);
  v.sync = registry_.sync(handle_);
  return v;
}

ConstFrameView FrameBundle::view() const noexcept {
  ConstFrameView v{};
  v.handle = handle_;
  v.cmd = registry_.cmd(handle_);
  v.sync = registry_.sync(handle_);
  return v;
}

details::FrameCmd *FrameBundle::cmd() noexcept {
  return registry_.cmd(handle_);
}

const details::FrameCmd *FrameBundle::cmd() const noexcept {
  return registry_.cmd(handle_);
}

details::FrameSync *FrameBundle::sync() noexcept {
  return registry_.sync(handle_);
}

const details::FrameSync *FrameBundle::sync() const noexcept {
  return registry_.sync(handle_);
}

void FrameBundle::retire(uint64_t retire_at) noexcept {
  if (!handle_.valid()) {
    return;
  }

  registry_.destroy(handle_, retire_at);
  handle_ = details::FrameHandle{};
}

} // namespace quark::vk
