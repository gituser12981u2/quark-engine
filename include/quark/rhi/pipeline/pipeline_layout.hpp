#pragma once

#include "quark/rhi/backend/native_backend_ref.hpp"
#include "quark/rhi/device/backend_access.hpp"
#include "quark/rhi/device/device_view.hpp"
#include "quark/rhi/pipeline/pipeline_layout_desc.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include <memory>

namespace quark::rhi {

class Pipeline;

class PipelineLayout final {
public:
  using Desc = PipelineLayoutDesc;

  struct CreateInfo {
    DeviceView device{};
    const Desc *desc{nullptr};
  };

  PipelineLayout();
  ~PipelineLayout();

  QUARK_MOVE_ONLY(PipelineLayout);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept;

private:
  friend class Pipeline;

  [[nodiscard]] NativeBackendRef native_backend() const noexcept;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace quark::rhi
