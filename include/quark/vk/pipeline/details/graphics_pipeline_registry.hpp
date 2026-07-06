#pragma once

#include "quark/engine/registry/registry_base.hpp"
#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include "quark/vk/device/device_view.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_handle.hpp"
#include "quark/vk/pipeline/details/pipeline.hpp"

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace quark {

namespace engine {
class RetirementQueue;
}

namespace rhi {
class PipelineLayout;

namespace details {
class ShaderRegistry;
}
} // namespace rhi

namespace vk {
struct GraphicsPipelineDesc;

namespace details {

struct GraphicsPipelineSlot {
  bool live{false};
  uint32_t generation{1};

  Pipeline pipeline;
  const rhi::PipelineLayout *layout{nullptr};
};

struct RetiredGraphicsPipeline {
  Pipeline pipeline;
};

struct GraphicsPipelineRegistryPolicy {
  static void destroy_slot_immediate(GraphicsPipelineSlot &slot) noexcept;
  static void
  move_slot_to_retired_payload(GraphicsPipelineSlot &slot,
                               RetiredGraphicsPipeline &payload) noexcept;
  static void destroy_retired_payload(void *ctx) noexcept;
  static void cleanup_retired_payload(void *ctx) noexcept;
};

class GraphicsPipelineRegistry final
    : private engine::RegistryBase<GraphicsPipelineHandle, GraphicsPipelineSlot,
                                   RetiredGraphicsPipeline,
                                   GraphicsPipelineRegistryPolicy> {
private:
  using Base =
      engine::RegistryBase<GraphicsPipelineHandle, GraphicsPipelineSlot,
                           RetiredGraphicsPipeline,
                           GraphicsPipelineRegistryPolicy>;

public:
  struct CreateInfo {
    DeviceView device{};
    const rhi::details::ShaderRegistry *shaders{nullptr};
    engine::RetirementQueue *retire_queue{nullptr};
    const VkAllocationCallbacks *allocator{nullptr};
  };

  struct PipelineCreateInfo {
    VkExtent2D extent{};
    const GraphicsPipelineDesc *desc{nullptr};
  };

  GraphicsPipelineRegistry() = default;
  ~GraphicsPipelineRegistry() { destroy(); }

  QUARK_MOVE_ONLY(GraphicsPipelineRegistry);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] util::Result<GraphicsPipelineHandle>
  create_pipeline(const PipelineCreateInfo &ci);

  void clear() noexcept;

  void destroy(GraphicsPipelineHandle handle, uint64_t retire_at) noexcept;

  using Base::alive;

  [[nodiscard]] const Pipeline *
  pipeline(GraphicsPipelineHandle handle) const noexcept;
  [[nodiscard]] const rhi::PipelineLayout *
  layout(GraphicsPipelineHandle handle) const noexcept;

private:
  DeviceView device_{};
  const rhi::details::ShaderRegistry *shaders_{nullptr};
  const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace details

} // namespace vk

} // namespace quark
