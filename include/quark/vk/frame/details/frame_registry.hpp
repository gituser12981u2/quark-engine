#pragma once

#include <cstdint>
#include <quark/engine/registry/registry_base.hpp>
#include <quark/engine/retire/retirement_queue.hpp>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/device_view.hpp>
#include <quark/vk/frame/details/frame_cmd.hpp>
#include <quark/vk/frame/details/frame_handle.hpp>
#include <quark/vk/frame/details/frame_sync.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

struct FrameRegistrySlot {
  bool live = false;
  uint32_t generation = 1;

  FrameCmd cmd;
  FrameSync sync;
};

struct FrameRetiredPayload {
  FrameCmd cmd;
  FrameSync sync;
};

struct FrameRegistryPolicy {
  static void destroy_slot_immediate(FrameRegistrySlot &slot) noexcept;
  static void
  move_slot_to_retired_payload(FrameRegistrySlot &slot,
                               FrameRetiredPayload &payload) noexcept;

  static void destroy_retired_payload(void *ctx) noexcept;
  static void cleanup_retired_payload(void *ctx) noexcept;
};

class FrameRegistry final
    : public engine::RegistryBase<FrameHandle, FrameRegistrySlot,
                                  FrameRetiredPayload, FrameRegistryPolicy> {
private:
  using Base = engine::RegistryBase<FrameHandle, FrameRegistrySlot,
                                    FrameRetiredPayload, FrameRegistryPolicy>;

public:
  struct CreateInfo {
    DeviceView device{};
    engine::RetirementQueue *retire_queue = nullptr;

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
  void destroy(FrameHandle handle, uint64_t retire_at) noexcept;

  // TODO: add validate()

  void clear() noexcept;

  using Base::alive;

  [[nodiscard]] FrameCmd *cmd(FrameHandle handle) noexcept;
  [[nodiscard]] const FrameCmd *cmd(FrameHandle handle) const noexcept;

  [[nodiscard]] FrameSync *sync(FrameHandle handle) noexcept;
  [[nodiscard]] const FrameSync *sync(FrameHandle handle) const noexcept;
};

} // namespace quark::vk::details
