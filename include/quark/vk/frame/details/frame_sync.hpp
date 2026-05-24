#pragma once

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/device_view.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

class FrameSync final {
public:
  struct CreateInfo {
    DeviceView device{};
    uint32_t frames_in_flight = 0;
    const VkAllocationCallbacks *allocator = nullptr;

    // If true, frames start idle
    // If false, one can pre-seed values
    bool frames_idle_on_create = true;
  };

  FrameSync() = default;
  ~FrameSync() { destroy(); }

  QUARK_MOVE_ONLY(FrameSync);

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  util::Status resize(uint32_t new_frames_in_flight);

  [[nodiscard]] bool valid() const noexcept { return !frames_.empty(); }

  [[nodiscard]] uint32_t frame_count() const noexcept {
    return static_cast<uint32_t>(frames_.size());
  }

  [[nodiscard]] VkSemaphore image_available(uint32_t frame) const noexcept {
    return (frame < frames_.size()) ? frames_[frame].image_available
                                    : VK_NULL_HANDLE;
  }

  [[nodiscard]] VkSemaphore render_finished(uint32_t frame) const noexcept {
    return (frame < frames_.size()) ? frames_[frame].render_finished
                                    : VK_NULL_HANDLE;
  }

  /**
   * Last submitted timeline value for this frame slot.
   *
   * 0 -> nothing submitted yet.
   */
  [[nodiscard]] uint64_t in_flight_value(uint32_t frame) const noexcept {
    return (frame < frames_.size()) ? frames_[frame].in_flight_value : 0;
  }

  /// After vkQueueSubmit, record the value that represents frame slot is in
  /// flight.
  void mark_submitted(uint32_t frame, uint64_t value) noexcept {
    if (frame < frames_.size()) {
      frames_[frame].in_flight_value = value;
    }
  }

private:
  struct PerFrame {
    VkSemaphore image_available = VK_NULL_HANDLE;
    VkSemaphore render_finished = VK_NULL_HANDLE;
    uint64_t in_flight_value = 0;
  };

  util::Status create_per_frame_(uint32_t count);
  void destroy_per_frame_() noexcept;

  VkDevice vk_device_ = VK_NULL_HANDLE; // non-owning
  const VkAllocationCallbacks *alloc_ = nullptr;

  bool frames_idle_on_create_ = true;
  std::vector<PerFrame> frames_;
};

} // namespace quark::vk::details
