#pragma once

#include "quark/engine/registry/registry_base.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/descriptor/descriptor_set_layout.hpp"
#include "quark/vk/descriptor/details/descriptor_set_layout_handle.hpp"
#include "quark/vk/device/device_view.hpp"
#include <cstdint>

namespace quark::vk {

class RetirementQueue;

namespace details {

struct DescriptorSetLayoutSlot {
  bool live{false};
  uint32_t generation{1};
  DescriptorSetLayout layout;
};

struct RetiredDescriptorSetLayout {
  DescriptorSetLayout layout;
};

struct DescriptorSetLayoutRegistryPolicy {
  static void destroy_slot_immediate(DescriptorSetLayoutSlot &slot) noexcept;

  static void
  move_slot_to_retired_payload(DescriptorSetLayoutSlot &slot,
                               RetiredDescriptorSetLayout &payload) noexcept;

  static void destroy_retired_payload(void *ctx) noexcept;
  static void cleanup_retired_payload(void *ctx) noexcept;
};

class DescriptorSetLayoutRegistry final
    : private engine::RegistryBase<
          DescriptorSetLayoutHandle, DescriptorSetLayoutSlot,
          RetiredDescriptorSetLayout, DescriptorSetLayoutRegistryPolicy> {
private:
  using Base =
      engine::RegistryBase<DescriptorSetLayoutHandle, DescriptorSetLayoutSlot,
                           RetiredDescriptorSetLayout,
                           DescriptorSetLayoutRegistryPolicy>;

public:
  struct CreateInfo {
    DeviceView device{};
    RetirementQueue *retire_queue{nullptr};
    const VkAllocationCallbacks *allocator{nullptr};
  };

  struct LayoutCreateInfo {
    DescriptorSetLayoutDesc desc{};
  };

  DescriptorSetLayoutRegistry() = default;
  ~DescriptorSetLayoutRegistry() { destroy(); }

  QUARK_MOVE_ONLY(DescriptorSetLayoutRegistry);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] util::Result<DescriptorSetLayoutHandle>
  create_layout(const LayoutCreateInfo &ci);

  void clear() noexcept;

  void destroy(DescriptorSetLayoutHandle handle, uint64_t retire_at) noexcept;

  using Base::alive;

  [[nodiscard]] VkDescriptorSetLayout
  layout(DescriptorSetLayoutHandle handle) const noexcept;

private:
  DeviceView device_{};
  const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace details

} // namespace quark::vk
