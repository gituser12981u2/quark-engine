#pragma once

#include "quark/engine/registry/registry_base.hpp"
#include "quark/engine/retire/retirement_queue.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/descriptor/details/descriptor_set_layout_handle.hpp"
#include "quark/vk/descriptor/details/descriptor_set_layout_registry.hpp"
#include "quark/vk/device/device_view.hpp"
#include "quark/vk/pipeline/details/pipeline_layout.hpp"
#include "quark/vk/pipeline/details/pipeline_layout_handle.hpp"
#include <cstdint>
#include <span>

namespace quark::vk {

class RetirementQueue;

namespace details {

struct PipelineLayoutSlot {
  bool live{false};
  uint32_t generation{1};
  PipelineLayout layout;
};

struct RetiredPipelineLayout {
  PipelineLayout layout;
};

struct PipelineLayoutRegistryPolicy {
  static void destroy_slot_immediate(PipelineLayoutSlot &slot) noexcept;

  static void
  move_slot_to_retired_payload(PipelineLayoutSlot &slot,
                               RetiredPipelineLayout &payload) noexcept;

  static void destroy_retired_payload(void *ctx) noexcept;
  static void cleanup_retired_payload(void *ctx) noexcept;
};

class PipelineLayoutRegistry final
    : private engine::RegistryBase<PipelineLayoutHandle, PipelineLayoutSlot,
                                   RetiredPipelineLayout,
                                   PipelineLayoutRegistryPolicy> {
private:
  using Base =
      engine::RegistryBase<PipelineLayoutHandle, PipelineLayoutSlot,
                           RetiredPipelineLayout, PipelineLayoutRegistryPolicy>;

public:
  struct CreateInfo {
    DeviceView device{};
    const DescriptorSetLayoutRegistry *descriptor_set_layouts{nullptr};
    RetirementQueue *retire_queue{nullptr};
    const VkAllocationCallbacks *allocator{nullptr};
  };

  struct LayoutCreateInfo {
    std::span<const DescriptorSetLayoutHandle> set_layouts;
    std::span<const VkPushConstantRange> push_constant_ranges;
  };

  PipelineLayoutRegistry() = default;
  ~PipelineLayoutRegistry() { destroy(); }

  QUARK_MOVE_ONLY(PipelineLayoutRegistry);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] util::Result<PipelineLayoutHandle>
  create_layout(const LayoutCreateInfo &ci);

  void clear() noexcept;

  void destroy(PipelineLayoutHandle handle, uint64_t retire_at) noexcept;

  using Base::alive;

  [[nodiscard]] VkPipelineLayout
  layout(PipelineLayoutHandle handle) const noexcept;

  [[nodiscard]] const PipelineLayout *
  backend(PipelineLayoutHandle handle) const noexcept;

private:
  DeviceView device_{};
  const DescriptorSetLayoutRegistry *descriptor_set_layouts_{nullptr};
  const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace details

} // namespace quark::vk
