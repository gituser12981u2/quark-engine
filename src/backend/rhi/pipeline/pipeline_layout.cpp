#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/rhi/backend/native_backend_ref.hpp"
#include "quark/rhi/device/backend_access.hpp"
#include "quark/rhi/device/device_view.hpp"
#include "quark/rhi/pipeline/details/pipeline_backend.hpp"
#include "quark/rhi/pipeline/details/pipeline_layout_handle.hpp"

#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"

#include <memory>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace quark::rhi {

struct PipelineLayout::Impl {
  DeviceView device{};
  details::PipelineLayoutHandle handle{};
};

PipelineLayout::PipelineLayout() = default;
PipelineLayout::~PipelineLayout() { destroy(); }

util::Status PipelineLayout::create(const CreateInfo &ci) {
  QUARK_ENSURE(
      ci.device.valid(),
      QUARK_ERR(util::Errc::InvalidArg, "pipeline layout device is invalid"));

  QUARK_ENSURE(ci.desc != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                             "pipeline layout desc is null"));

  const vk::DeviceView device = ci.device.backend();

  QUARK_TRY_STATUS(vk::validate(device));

  const details::PipelineLayoutBackendCreateInfo backend_ci{
      .device = ci.device,
      .desc = ci.desc,
      .retire_queue = ci.retire_queue,
  };

  QUARK_TRY_ASSIGN(new_impl->handle,
                   details::backend::create_pipeline_layout(backend_ci));

  destroy();
  impl_ = std::move(new_impl);

  QUARK_OK();
}

void PipelineLayout::destroy() noexcept {
  if (impl_ == nullptr) {
    return;
  }

  if (impl_->device.valid() && impl_->handle.valid()) {
    details::backend::destroy_pipeline_layout(impl_->device, impl_->handle);
  }

  impl_.reset();
}

bool PipelineLayout::valid() const noexcept {
  return impl_ != nullptr && impl_->device.valid() && impl_->handle.valid() &&
         details::backend::pipeline_layout_alive(impl_->device, impl_->handle);
}

NativeBackendRef PipelineLayout::native_backend() const noexcept {
  if (!valid()) {
    return {};
  }

  return details::backend::pipeline_layout_native_backend(impl_->device,
                                                          impl_->handle);
}

} // namespace quark::rhi
