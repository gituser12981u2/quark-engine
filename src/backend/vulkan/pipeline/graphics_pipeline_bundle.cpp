#include "quark/vk/pipeline/graphics_pipeline_bundle.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_handle.hpp"
#include "quark/vk/pipeline/details/graphics_pipeline_registry.hpp"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status GraphicsPipelineBundle::create(const CreateInfo &ci) {
  destroy();

  details::GraphicsPipelineRegistry::CreateInfo registry_ci{
      .device = ci.device,
      .shaders = ci.shaders,
      .retire_queue = ci.retire_queue,
      .allocator = ci.allocator,
  };

  QUARK_TRY_STATUS(registry_.create(registry_ci));

  details::GraphicsPipelineRegistry::PipelineCreateInfo pipeline_ci{
      .extent = ci.extent,
      .desc = ci.desc,
  };

  QUARK_TRY_ASSIGN(handle_, registry_.create_pipeline(pipeline_ci));
  QUARK_OK();
}

void GraphicsPipelineBundle::destroy() noexcept {
  if (!handle_.valid()) {
    return;
  }

  registry_.clear();
  handle_ = details::GraphicsPipelineHandle{};
}

VkPipeline GraphicsPipelineBundle::pipeline() const noexcept {
  return registry_.pipeline(handle_);
}

VkPipelineLayout GraphicsPipelineBundle::layout() const noexcept {
  return registry_.layout(handle_);
}

// GraphicsPipelineView GraphicsPipelineBundle::view() const noexcept {
//   return GraphicsPipelineView{
//       .pipeline = pipeline(),
//       .layout = layout(),
//   };
// }

void GraphicsPipelineBundle::retire(uint64_t retire_at) noexcept {
  if (!handle_.valid()) {
    return;
  }

  registry_.destroy(handle_, retire_at);
  handle_ = details::GraphicsPipelineHandle{};
}

} // namespace quark::vk
