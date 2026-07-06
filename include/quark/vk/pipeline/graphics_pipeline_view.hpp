#pragma once

#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_handle.hpp"
#include "quark/vk/pipeline/details/pipeline.hpp"

namespace quark::vk {

struct GraphicsPipelineView {
  details::GraphicsPipelineHandle handle{};
  const details::Pipeline *pipeline{nullptr};
  const rhi::PipelineLayout *layout{nullptr};

  [[nodiscard]] bool valid() const noexcept {
    return handle.valid() && pipeline != VK_NULL_HANDLE && layout != nullptr &&
           layout->valid();
  }

  [[nodiscard]] VkPipeline vk_pipeline() const noexcept {
    return valid() ? pipeline->handle() : VK_NULL_HANDLE;
  }
};

} // namespace quark::vk
