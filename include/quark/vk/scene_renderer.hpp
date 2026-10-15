#pragma once

#include <array>
#include <chrono>

#include <quark/engine/retire/retirement_queue.hpp>
#include <quark/platform/shader/details/shader_registry.hpp>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/allocator.hpp>
#include <quark/vk/device/device_view.hpp>
#include <quark/vk/mesh.hpp>
#include <quark/vk/pipeline/graphics_pipeline_bundle.hpp>
#include <quark/vk/pipeline/graphics_pipeline_desc.hpp>
#include <quark/vk/pipeline/vertex_layout.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class SceneRenderer final {
public:
  struct CreateInfo {
    DeviceView device{};
    const Allocator *allocator = nullptr;
    RetirementQueue *retire_queue = nullptr;
    VkExtent2D extent{};
    VkFormat color_format{VK_FORMAT_UNDEFINED};
    VkFormat depth_format{VK_FORMAT_UNDEFINED};
    PipelineRenderBackend backend{PipelineRenderBackend::DynamicRendering};
    VkRenderPass render_pass{VK_NULL_HANDLE};
  };

  SceneRenderer() = default;
  ~SceneRenderer() { destroy(); }

  QUARK_NO_COPY_NO_MOVE(SceneRenderer);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  void draw(VkCommandBuffer command_buffer, VkExtent2D extent,
            std::chrono::steady_clock::duration elapsed) const noexcept;

private:
  struct Object { // Too generic a name?
    std::array<float, 3> position;
    std::array<float, 3> rotation;
    std::array<float, 3> scale;
    std::array<float, 4> tint;
  };

  GraphicsPipelineBundle pipeline_;
  ShaderRegistry shader_registry_;
  Mesh cube_;
  std::array<Object, 3> objects_{
      {
          {
              .position = {-1.45F, 0.0F, 0.0F},
              .rotation = {0.35F, 0.55F, 0.0F},
              .scale = {0.82F, 0.82F, 0.82F},
              .tint = {1.0F, 0.48F, 0.34F, 1.0F},
          },
          {
              .position = {0.0F, 0.0F, -0.85F},
              .rotation = {0.65F, -0.35F, 0.0F},
              .scale = {0.92F, 0.92F, 0.92F},
              .tint = {0.38F, 0.78F, 1.0F, 1.0F},
          },
          {
              .position = {1.45F, 0.0F, 0.0F},
              .rotation = {-0.25F, 0.7F, 0.0F},
              .scale = {0.72F, 0.72F, 0.72F},
              .tint = {0.58F, 1.0F, 0.48F, 1.0F},
          },
      },
  };
};

} // namespace quark::vk
