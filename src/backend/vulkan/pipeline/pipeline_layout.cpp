#include "quark/vk/pipeline/pipeline_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status PipelineLayout::create(const CreateInfo &ci) {
  QUARK_ENSURE(ci.desc != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                             "pipeline layout desc is null"));

  destroy();

  QUARK_TRY_STATUS(registry_.create({
      .device = ci.device,
      .descriptor_set_layouts = ci.descriptor_set_layouts,
      .retire_queue = ci.retire_queue,
      .allocator = ci.allocator,
  }));

  QUARK_TRY_ASSIGN(handle_,
                   registry_.create_layout({
                       .set_layouts = ci.desc->descriptor_set_layouts,
                       .push_constant_ranges = ci.desc->push_constant_ranges,
                   }));

  backend_ = registry_.backend(handle_);

  QUARK_OK();
}

void PipelineLayout::destroy() noexcept {
  registry_.destroy();
  handle_ = {};
  backend_ = nullptr;
}

} // namespace quark::vk
