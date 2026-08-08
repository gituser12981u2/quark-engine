#include "quark/rhi/pipeline/details/pipeline.hpp"
#include "quark/engine/registry/backend_registry.hpp"
#include "quark/rhi/backend/native_backend_ref.hpp"
#include "quark/rhi/pipeline/details/pipeline_handle.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"

#include "quark/vk/pipeline/pipeline_backend.hpp"

#include <memory>
#include <variant>

namespace quark::rhi {

struct Pipeline::Impl {
  using Backend = std::variant<vk::PipelineBackend>;

  engine::BackendRegistry<details::PipelineHandle, Backend> registry;
  details::PipelineHandle handle{};
};

Pipeline::Pipeline() = default;
Pipeline::~Pipeline() = default;

util::Status Pipeline::create_graphics(const GraphicsCreateInfo &ci) {
  QUARK_ENSURE(ci.desc != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                             "graphics pipeline desc is null"));

  QUARK_ENSURE(ci.shaders != nullptr,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline shader registry is null"));

  QUARK_ENSURE(
      ci.pipeline_layout != nullptr && ci.pipeline_layout->valid(),
      QUARK_ERR(util::Errc::InvalidArg, "graphics pipeline layout is invalid"));

  destroy();

  impl_ = std::make_unique<Impl>();

  QUARK_TRY_STATUS(impl_->registry.create({
      .retire_queue = ci.retire_queue,
  }));

  const vk::PipelineBackend::GraphicsCreateInfo backend_ci{
      .device = ci.device,
      .desc = ci.desc,
      .shaders = ci.shaders,
      .pipeline_layout = ci.pipeline_layout,
  };

  QUARK_TRY_ASSIGN(impl_->handle, impl_->registry.create_backend(backend_ci));

  QUARK_OK();
}

void Pipeline::destroy() noexcept {
  if (impl_ == nullptr) {
    return;
  }

  impl_->registry.destroy();
  impl_.reset();
}

bool Pipeline::valid() const noexcept {
  return impl_ != nullptr && impl_->handle.valid() &&
         impl_->registry.alive(impl_->handle);
}

NativeBackendRef Pipeline::native_backend() const noexcept {
  if (!valid()) {
    return {};
  }

  return NativeBackendRef{.ptr = impl_->registry.backend(impl_->handle)};
}

} // namespace quark::rhi
