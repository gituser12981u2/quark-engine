#pragma once

#include "quark/engine/retire/retirement_queue.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_handle.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_registry.hpp"
#include "quark/vk/pipeline/graphics_pipeline_desc.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class GraphicsPipelineBundle final {
public:
  struct CreateInfo {
    DeviceView device{};
    const ShaderRegistry *shaders{nullptr};
    RetirementQueue *retire_queue{nullptr};
    const VkAllocationCallbacks *allocator{nullptr};

    VkExtent2D extent{};
    const GraphicsPipelineDesc *desc{nullptr};
  };

  GraphicsPipelineBundle() = default;
  ~GraphicsPipelineBundle() { destroy(); }

  QUARK_MOVE_ONLY(GraphicsPipelineBundle);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return registry_.alive(handle_); }

  [[nodiscard]] VkPipeline pipeline() const noexcept;
  [[nodiscard]] VkPipelineLayout layout() const noexcept;

  // [[nodiscard]] GraphicsPipelineView view() const noexcept;

  void retire(uint64_t retire_at) noexcept;

private:
  details::GraphicsPipelineRegistry registry_;
  details::GraphicsPipelineHandle handle_{};
};

} // namespace quark::vk
