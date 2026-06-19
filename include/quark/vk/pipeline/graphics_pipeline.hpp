#pragma once

#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_handle.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_registry.hpp"
#include "quark/vk/pipeline/details/pipeline.hpp"
#include "quark/vk/pipeline/graphics_pipeline_view.hpp"
#include "quark/vk/pipeline/pipeline_layout.hpp"
#include <cstdint>

namespace quark::vk {

class ShaderRegistry;
class RetirementQueue;
struct GraphicsPipelineDesc;

class GraphicsPipeline final {
public:
  struct CreateInfo {
    DeviceView device{};
    VkExtent2D extent{};
    const ShaderRegistry *shaders{nullptr};
    RetirementQueue *retire_queue{nullptr};
    const GraphicsPipelineDesc *desc{nullptr};
    const VkAllocationCallbacks *allocator{nullptr};
  };

  GraphicsPipeline() = default;
  ~GraphicsPipeline() { destroy(); }

  QUARK_MOVE_ONLY(GraphicsPipeline);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return registry_.alive(handle_); }

  [[nodiscard]] const details::Pipeline *pipeline() const noexcept;
  [[nodiscard]] const PipelineLayout *layout() const noexcept;
  [[nodiscard]] GraphicsPipelineView view() const noexcept;

  void retire(uint64_t retire_at) noexcept;

private:
  details::GraphicsPipelineRegistry registry_;
  details::GraphicsPipelineHandle handle_{};
};

} // namespace quark::vk
