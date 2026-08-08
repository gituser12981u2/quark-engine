#pragma once

#include "quark/rhi/pipeline/details/pipeline_handle.hpp"
#include "quark/rhi/pipeline/details/pipeline_layout_handle.hpp"
#include "quark/vk/device/device_view.hpp"

namespace quark::rhi {

class DeviceView;
class Pipeline;
class PipelineLayout;

namespace details {

class BackendAccess final {
public:
  BackendAccess() = delete;

  [[nodiscard]] static vk::DeviceView native_device(DeviceView device) noexcept;

  [[nodiscard]] static DeviceView device(const PipelineLayout &layout) noexcept;

  [[nodiscard]] static PipelineLayoutHandle
  handle(const PipelineLayout &layout) noexcept;

  [[nodiscard]] static DeviceView device(const Pipeline &pipeline) noexcept;

  [[nodiscard]] static PipelineHandle handle(const Pipeline &pipeline) noexcept;
};

} // namespace details

} // namespace quark::rhi
