#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"

#include "quark/vk/pipeline/pipeline_layout_backend.hpp"

namespace quark::rhi {

util::Status PipelineLayout::create(const CreateInfo &ci) {
  QUARK_ENSURE(ci.desc != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                             "pipeline layout desc is null"));

  destroy();

  QUARK_TRY_STATUS(registry_.create({
      .retire_queue = ci.retire_queue,
  }));

  // TODO: remove this pattern
  const vk::PipelineLayoutBackend::CreateInfo backend_ci{
      .device = ci.device,
      .desc = ci.desc,
  };

  QUARK_TRY_ASSIGN(handle_, registry_.create_backend(backend_ci));

  backend_ = registry_.backend(handle_);

  QUARK_OK();
}

void PipelineLayout::destroy() noexcept {
  registry_.destroy();
  handle_ = {};
  backend_ = nullptr;
}

} // namespace quark::rhi
