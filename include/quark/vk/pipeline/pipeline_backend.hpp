#pragma once

#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"

#include <vulkan/vulkan_core.h>

namespace quark {

namespace rhi {
class PipelineLayout;
}

namespace rhi::details {
class ShaderRegistry;
}

namespace vk {

struct GraphicsPipelineDesc;

class PipelineBackend final {
public:
  struct GraphicsCreateInfo {
    DeviceView device{};
    const vk::GraphicsPipelineDesc *desc{nullptr};
    const rhi::details::ShaderRegistry *shaders{nullptr};
    const rhi::PipelineLayout *pipeline_layout{nullptr};
    VkPipelineCache cache{VK_NULL_HANDLE};
    const VkAllocationCallbacks *allocator{nullptr};
  };

  PipelineBackend() = default;
  ~PipelineBackend() { destroy(); }

  QUARK_MOVE_ONLY(PipelineBackend);

  [[nodiscard]] util::Status create_graphics(const GraphicsCreateInfo &ci);

  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPipeline vk_handle() const noexcept { return handle_; }

private:
  DeviceView device_{};
  VkPipeline handle_{VK_NULL_HANDLE};
  const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace vk

} // namespace quark
