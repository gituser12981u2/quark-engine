#pragma once

#include "quark/engine/retire/retirement_queue.hpp"
#include "quark/rhi/backend/native_backend_ref.hpp"
#include "quark/rhi/device/device_view.hpp"
#include "quark/rhi/pipeline/details/pipeline_layout_handle.hpp"
#include "quark/rhi/pipeline/pipeline_layout_desc.hpp"
#include "quark/utils/result.hpp"

namespace quark {

namespace engine {
class RetirementQueue;
}

namespace rhi {
struct PipelineLayoutDesc;

namespace details {

struct PipelineLayoutBackendCreateInfo {
  DeviceView device;
  const PipelineLayoutDesc *desc{nullptr};

  engine::RetirementQueue *retire_queue{nullptr};
};

namespace backend {

[[nodiscard]] util::Result<PipelineLayoutHandle>
create_pipeline_layout(const PipelineLayoutBackendCreateInfo &ci);

void destroy_pipeline_layout(DeviceView device,
                             PipelineLayoutHandle layout) noexcept;

[[nodiscard]] bool pipeline_layout_alive(DeviceView device,
                                         PipelineLayoutHandle layout) noexcept;

[[nodiscard]] NativeBackendRef
pipeline_layout_native_backend(DeviceView device,
                               PipelineLayoutHandle layout) noexcept;
} // namespace backend
} // namespace details
} // namespace rhi

} // namespace quark
