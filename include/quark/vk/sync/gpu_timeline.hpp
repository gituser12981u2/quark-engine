#pragma once

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/device_view.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class GpuTimeline final {
public:
  struct CreateInfo {
    DeviceView device{};
    const VkAllocationCallbacks *allocator = nullptr;

    /// Initial semaphore counter value.
    uint64_t initial_value = 0;

    /// First value returned by next_signal_value().
    uint64_t first_signal_value = 0;
  };

  GpuTimeline() = default;
  ~GpuTimeline() { destroy(); }

  QUARK_MOVE_ONLY(GpuTimeline);

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  // TODO: make into a validate
  [[nodiscard]] bool valid() const noexcept {
    return vk_device_ != VK_NULL_HANDLE && timeline_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkSemaphore semaphore() const noexcept { return timeline_; }

  /**
   * @brief Returns a unique value to be signaled on the next submission.
   *
   * Contract:
   * - Values are monotonically increasing.
   * - Caller must ensure submissions signal the returned value on this
   * timeline.
   */
  [[nodiscard]] uint64_t next_signal_value() noexcept { return ++next_value_; }

  /**
   * @brief Returns the current completed counter value of the timeline.
   */
  [[nodiscard]] util::Result<uint64_t> completed_value() const noexcept;

  // Wait until completed >= value.
  //

  /**
   * @brief Wait until the timeline has reached at least @p value.
   *
   * No-op if @p value == 0.
   *
   * @param value The target counter value.
   * @param timeout_ns Timeout in nanoseconds; UINT64_MAX waits indefinitely.
   */
  [[nodiscard]] util::Status wait(uint64_t value,
                                  uint64_t timeout_ns = UINT64_MAX) const;

private:
  VkDevice vk_device_ = VK_NULL_HANDLE; // non-owning
  const VkAllocationCallbacks *alloc_ = nullptr;

  VkSemaphore timeline_ = VK_NULL_HANDLE;

  uint64_t next_value_ = 1;
};

} // namespace quark::vk
