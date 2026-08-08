#pragma once

#include "quark/engine/retire/retirement_queue.hpp"
#include "quark/rhi/backend/native_backend_ref.hpp"
#include "quark/rhi/pipeline/details/pipeline_handle.hpp"
#include "quark/rhi/pipeline/graphics_pipeline_desc.hpp"
#include "quark/rhi/shader/details/shader_registry.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include "quark/vk/device/device_view.hpp"

#include <vulkan/vulkan_core.h>

namespace quark::rhi {

struct GraphicsPipelineDesc;

namespace details {
class ShaderRegistry;
}

class Pipeline final {
public:
  struct GraphicsCreateInfo {
    vk::DeviceView device{};
    const GraphicsPipelineDesc *desc{nullptr};
    const details::ShaderRegistry *shaders{nullptr};
    engine::RetirementQueue *retire_queue{nullptr};
  };

  Pipeline();
  ~Pipeline();

  QUARK_MOVE_ONLY(Pipeline);

  [[nodiscard]] util::Status create_graphics(const GraphicsCreateInfo &ci);

  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept;

private:
  [[nodiscard]] NativeBackendRef native_backend() const noexcept;

  vk::DeviceView device_{};
  details::PipelineHandle handle_{};
  engine::RetirementQueue *retire_queue_{nullptr};
};

} // namespace quark::rhi
