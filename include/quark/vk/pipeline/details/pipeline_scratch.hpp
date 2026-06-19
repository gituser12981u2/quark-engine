#pragma once

#include "quark/utils/raii.hpp"
#include "quark/vk/pipeline/pipeline_limits.hpp"
#include <array>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

// TODO: replace with inplace_vector and upgrade to c++26
class PipelineScratch final {
public:
  explicit PipelineScratch(VkDevice device) noexcept : device_(device) {}
  ~PipelineScratch() { destroy_shader_modules(); }

  QUARK_NO_COPY_NO_MOVE(PipelineScratch);

  std::array<VkPipelineShaderStageCreateInfo,
             pipeline_limits::kMaxGraphicsShaderStages>
      stages{};

  std::array<VkShaderModule, pipeline_limits::kMaxGraphicsShaderStages>
      shader_modules{};

  std::array<VkVertexInputBindingDescription,
             pipeline_limits::kMaxVertexBindings>
      bindings{};

  std::array<VkVertexInputAttributeDescription,
             pipeline_limits::kMaxVertexAttributes>
      attributes{};

  std::array<VkPipelineColorBlendAttachmentState,
             pipeline_limits::kMaxColorAttachments>
      color_blend_attachments{};

  std::array<VkFormat, pipeline_limits::kMaxColorAttachments> color_formats{};

  uint32_t stage_count{};
  uint32_t binding_count{};
  uint32_t attribute_count{};
  uint32_t color_attachments_count{};

private:
  void destroy_shader_modules() noexcept {
    if (device_ == VK_NULL_HANDLE) {
      return;
    }

    for (VkShaderModule &module : shader_modules) {
      if (module == VK_NULL_HANDLE) {
        continue;
      }

      vkDestroyShaderModule(device_, module, nullptr);
      module = VK_NULL_HANDLE;
    }
  }

  VkDevice device_{VK_NULL_HANDLE};
};

} // namespace quark::vk::details
