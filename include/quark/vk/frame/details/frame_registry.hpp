#pragma once

// Can we make forward declare friendly?
#include "quark/engine/retire/retirement_queue.hpp"
#include "quark/vk/device/device_view.hpp"

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/frame/details/frame_cmd.hpp>
#include <quark/vk/frame/details/frame_sync.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

struct FrameHandle;

class FrameRegistry final {
public:
  struct CreateInfo {
    DeviceView device{};
    RetirementQueue *retire_queue = nullptr;

    uint32_t frames_in_flight = 0;
    uint32_t cmd_buffer_count = 0;

    VkCommandPoolCreateFlags cmd_pool_flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    const VkAllocationCallbacks *allocator = nullptr;
    bool frames_idle_on_create = true;
  };

  FrameRegistry() = default;
  ~FrameRegistry() { clear(); }

  QUARK_MOVE_ONLY(FrameRegistry);

  [[nodiscard]] util::Result<FrameHandle> create(const CreateInfo &ci);
  void destory(FrameHandle handle, uint64_t retire_at) noexcept;

  // TODO: add validate()

  void clear() noexcept;

  [[nodiscard]] bool alive(FrameHandle handle) const noexcept;

  [[nodiscard]] FrameCmd *cmd(FrameHandle handle) noexcept;
  [[nodiscard]] const FrameCmd *cmd(FrameHandle handle) const noexcept;

  [[nodiscard]] FrameSync *sync(FrameHandle handle) noexcept;
  [[nodiscard]] const FrameSync *sync(FrameHandle handle) const noexcept;

private:
  struct Slot {
    bool live = false;
    uint32_t generation = 1;

    FrameCmd cmd;
    FrameSync sync;
  };

  struct RetiredFramePayload {
    FrameCmd cmd;
    FrameSync sync;
  };

  static void destroy_retired_frame_(void *ctx) noexcept;
  static void cleanup_retired_frame_(void *ctx) noexcept;

  [[nodiscard]] static bool matches_(FrameHandle handle,
                                     const Slot &slot) noexcept;

  RetirementQueue *retire_queue_ = nullptr;

  std::vector<Slot> slots_;
  std::vector<uint32_t> free_;
};

} // namespace quark::vk::details
