#pragma once

#include "quark/rhi/pipeline/details/pipeline_layout_handle.hpp"
#include "quark/rhi/pipeline/pipeline_layout_desc.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include "quark/vk/device/device_view.hpp"
#include "quark/vk/pipeline/pipeline_layout_backend.hpp"

namespace quark {

namespace engine {
class RetirementQueue;
}

namespace vk {
class GraphicsPipeline;

namespace details {
class Pipeline;
}

} // namespace vk

namespace rhi {

class PipelineLayout final {
public:
  using Desc = PipelineLayoutDesc;

  struct CreateInfo {
    vk::DeviceView device{};
    engine::RetirementQueue *retire_queue{nullptr};
    const Desc *desc{nullptr};
  };

  PipelineLayout() = default;
  ~PipelineLayout() { destroy(); }

  QUARK_MOVE_ONLY(PipelineLayout);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_.valid() && backend_ != nullptr && backend_->valid();
  }

private:
  friend class vk::GraphicsPipeline;
  friend class vk::details::Pipeline;

  [[nodiscard]] VkPipelineLayout vk_handle() const noexcept {
    return valid() ? backend_->handle() : VK_NULL_HANDLE;
  }

  using Registry = engine::BackendRegistry<details::PipelineLayoutHandle,
                                           vk::PipelineLayoutBackend>;

  Registry registry_;
  details::PipelineLayoutHandle handle_{};
  const vk::PipelineLayoutBackend *backend_{nullptr};
};

} // namespace rhi

} // namespace quark
