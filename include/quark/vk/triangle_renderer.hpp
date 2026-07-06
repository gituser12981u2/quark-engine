#pragma once

#include "quark/rhi/descriptor/descriptor_set_layout.hpp"
#include "quark/rhi/pipeline/pipeline_layout.hpp"
#include "quark/rhi/shader/details/shader_registry.hpp"

#include "quark/vk/pipeline/graphics_pipeline.hpp"
#include <quark/vk/allocator.hpp>
#include <quark/vk/device/device_view.hpp>
#include <quark/vk/pipeline/graphics_pipeline_desc.hpp>
#include <quark/vk/triangle_vertex_buffer.hpp>

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vulkan/vulkan_core.h>

namespace quark {

namespace engine {
class RetirementQueue;
}

namespace vk {
class TriangleRenderer final {
public:
  struct CreateInfo {
    DeviceView device{};
    const Allocator *allocator = nullptr;
    engine::RetirementQueue *retire_queue = nullptr;
    VkExtent2D extent{};
    VkFormat color_format{VK_FORMAT_UNDEFINED};
    PipelineRenderBackend backend{PipelineRenderBackend::DynamicRendering};
    VkRenderPass render_pass{VK_NULL_HANDLE};
  };

  TriangleRenderer() = default;
  ~TriangleRenderer() { destroy(); }

  QUARK_NO_COPY_NO_MOVE(TriangleRenderer);

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  void draw(VkCommandBuffer command_buffer, VkExtent2D extent) const noexcept;

private:
  rhi::PipelineLayout pipeline_layout_;
  GraphicsPipeline pipeline_;
  rhi::details::ShaderRegistry shader_registry_;
  TriangleVertexBuffer vertex_buffer_;

  rhi::DescriptorSetLayout camera_set_layout_;
};

} // namespace vk

} // namespace quark
