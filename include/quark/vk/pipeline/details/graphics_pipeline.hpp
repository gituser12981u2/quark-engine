#pragma once

#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"
#include "quark/vk/pipeline/details/pipeline.hpp"
#include "quark/vk/pipeline/details/pipeline_layout.hpp"

#include <vulkan/vulkan_core.h>

namespace quark::vk {

struct GraphicsPipelineDesc;
class ShaderRegistry;

namespace details {

class GraphicsPipeline final {
public:
  struct CreateInfo {
    DeviceView device{};
    VkExtent2D extent{};
    const GraphicsPipelineDesc *desc{nullptr};
    const ShaderRegistry *shaders{nullptr};
    const VkAllocationCallbacks *allocator{nullptr};
  };

  GraphicsPipeline() = default;
  ~GraphicsPipeline() { destroy(); }

  QUARK_MOVE_ONLY(GraphicsPipeline);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return pipeline_.valid(); }

  [[nodiscard]] VkPipeline pipeline() const noexcept {
    return pipeline_.handle();
  }
  [[nodiscard]] VkPipelineLayout layout() const noexcept {
    return layout_.handle();
  }

private:
  details::PipelineLayout layout_;
  details::Pipeline pipeline_;
};

} // namespace details
} // namespace quark::vk
