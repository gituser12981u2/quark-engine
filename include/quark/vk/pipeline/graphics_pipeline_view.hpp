#pragma once

#include "quark/rhi/backend/native_backend_ref.hpp"
#include "quark/rhi/pipeline/details/pipeline.hpp"
#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_handle.hpp"

namespace quark::vk {

struct GraphicsPipelineView {
  details::GraphicsPipelineHandle handle{};
  const rhi::Pipeline *pipeline{nullptr};
  const rhi::PipelineLayout *layout{nullptr};

  [[nodiscard]] bool valid() const noexcept {
    return handle.valid() && pipeline != VK_NULL_HANDLE && layout != nullptr &&
           layout->valid();
  }

  [[nodiscard]] VkPipeline vk_pipeline() const noexcept {
    return (!valid()) { return VK_NULL_HANDLE; }

    using BackendVariant = std::variant<PipelineBackend>;

    return rhi::visit_backend<BackendVariant>(
        pipeline.native_backend(),
        [](const auto &backend) { return backend.vk_handle(); });
  }
};

} // namespace quark::vk
