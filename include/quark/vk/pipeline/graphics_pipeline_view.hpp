#pragma once

#include "quark/vk/pipeline/details/graphics_pipeline_handle.hpp"
#include "quark/vk/pipeline/pipeline_layout.hpp"

namespace quark::vk {

struct GraphicsPipelineView {
  details::GraphicsPipelineHandle handle{};
  VkPipeline pipeline{VK_NULL_HANDLE};
  const PipelineLayout *layout{nullptr};

  [[nodiscard]] bool valid() const noexcept {
    return handle.valid() && pipeline != VK_NULL_HANDLE && layout != nullptr &&
           layout->valid();
  }
};

} // namespace quark::vk
