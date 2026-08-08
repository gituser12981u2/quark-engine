#include "quark/vk/pipeline/graphics_pipeline.hpp"
#include "quark/rhi/pipeline/details/pipeline.hpp"
#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/pipeline/graphics_pipeline_desc.hpp"
#include "quark/vk/pipeline/graphics_pipeline_view.hpp"

namespace quark::vk {

util::Status GraphicsPipeline::create(const CreateInfo &ci) {
  QUARK_ENSURE(ci.desc != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                             "graphics pipeline desc is null"));

  QUARK_ENSURE(
      ci.desc->pipeline_layout != nullptr && ci.desc->pipeline_layout->valid(),
      QUARK_ERR(util::Errc::InvalidArg, "graphics pipeline layout is invalid"));

  destroy();

  QUARK_TRY_STATUS(registry_.create({
      .device = ci.device,
      .shaders = ci.shaders,
      .retire_queue = ci.retire_queue,
      .allocator = ci.allocator,
  }));

  QUARK_TRY_ASSIGN(handle_, registry_.create_pipeline({
                                .extent = ci.extent,
                                .desc = ci.desc,
                            }));

  QUARK_OK();
}

void GraphicsPipeline::destroy() noexcept {
  registry_.destroy();
  handle_ = {};
}

const details::Pipeline *GraphicsPipeline::pipeline() const noexcept {
  return registry_.pipeline(handle_);
}

const rhi::PipelineLayout *GraphicsPipeline::layout() const noexcept {
  return registry_.layout(handle_);
}

GraphicsPipelineView GraphicsPipeline::view() const noexcept {
  return GraphicsPipelineView{
      .handle = handle_,
      .pipeline = registry_.pipeline(handle_),
      .layout = registry_.layout(handle_),
  };
}

} // namespace quark::vk
