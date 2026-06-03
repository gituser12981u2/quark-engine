#pragma once

#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include <vulkan/vulkan_core.h>

namespace quark::vk {

struct GraphicsPipelineDesc;

class GraphicsPipeline final {
public:
  struct CreateInfo {
    VkDevice device{VK_NULL_HANDLE};
    VkExtent2D extent{};
    const GraphicsPipelineDesc *desc{nullptr};
  };

  GraphicsPipeline() = default;
  ~GraphicsPipeline() { destroy(); }

  QUARK_MOVE_ONLY(GraphicsPipeline);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return device_ != VK_NULL_HANDLE && pipeline_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPipeline pipeline() const noexcept { return pipeline_; }
  [[nodiscard]] VkPipelineLayout layout() const noexcept { return layout_; }

private:
  VkDevice device_{VK_NULL_HANDLE};
  VkPipeline pipeline_{VK_NULL_HANDLE};
  VkPipelineLayout layout_{VK_NULL_HANDLE};
};

} // namespace quark::vk
