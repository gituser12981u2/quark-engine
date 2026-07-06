#pragma once

#include "quark/rhi/shader/details/shader_registry.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"

namespace quark {

namespace rhi {
class PipelineLayout;

namespace details {
class ShaderRegistry;
}
} // namespace rhi

namespace vk {

struct GraphicsPipelineDesc;

namespace details {

class Pipeline final {
public:
  struct CreateInfo {
    DeviceView device{};
    VkPipelineCache cache{VK_NULL_HANDLE};
    const VkGraphicsPipelineCreateInfo *graphics_info{nullptr};
    const VkAllocationCallbacks *allocator{nullptr};
  };

  struct GraphicsCreateInfo {
    DeviceView device{};
    VkExtent2D extent{};
    const GraphicsPipelineDesc *desc{nullptr};
    const rhi::details::ShaderRegistry *shaders{nullptr};
    const rhi::PipelineLayout *pipeline_layout{nullptr};
    VkPipelineCache cache{VK_NULL_HANDLE};
    const VkAllocationCallbacks *allocator{nullptr};
  };

  Pipeline() = default;
  ~Pipeline() { destroy(); }

  QUARK_MOVE_ONLY(Pipeline);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  [[nodiscard]] util::Status create_graphics(const GraphicsCreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPipeline handle() const noexcept { return handle_; }

private:
  DeviceView device_{};
  VkPipeline handle_{VK_NULL_HANDLE};
  const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace details
} // namespace vk

} // namespace quark
