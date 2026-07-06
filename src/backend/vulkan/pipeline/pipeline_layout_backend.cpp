#include "quark/vk/pipeline/pipeline_layout_backend.hpp"
#include "quark/rhi/descriptor/descriptor_set_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/vk/diagnostic_prelude.hpp"
#include "quark/vk/rhi_translation.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status PipelineLayoutBackend::create(const CreateInfo &ci) {
  QUARK_ENSURE(ci.desc != nullptr, QUARK_ERR(util::Errc::InvalidArg,
                                             "pipeline layout desc is null"));

  QUARK_TRY_STATUS(validate(ci.device));

  destroy();

  device_ = ci.device;

  std::vector<VkDescriptorSetLayout> vk_sets_layouts;
  vk_sets_layouts.reserve(ci.desc->descriptor_set_layouts.size());

  for (const rhi::DescriptorSetLayout *layout :
       ci.desc->descriptor_set_layouts) {
    QUARK_ENSURE(layout != nullptr,
                 QUARK_ERR(util::Errc::InvalidArg,
                           "pipeline layout descriptor set layout is null"));

    QUARK_ENSURE(layout->valid(),
                 QUARK_ERR(util::Errc::InvalidArg,
                           "pipeline layout descriptor set layout is invalid"));

    vk_sets_layouts.push_back(layout->vk_handle());
  }

  std::vector<VkPushConstantRange> vk_push_constant_ranges;
  vk_push_constant_ranges.reserve(ci.desc->push_constant_ranges.size());

  for (const rhi::PushConstantRange &range : ci.desc->push_constant_ranges) {
    const VkShaderStageFlags stages = to_vk_shader_stages(range.stages);

    QUARK_ENSURE(stages != 0,
                 QUARK_ERR(util::Errc::InvalidArg,
                           "push constant range shader stages are empty"));

    vk_push_constant_ranges.push_back(VkPushConstantRange{
        .stageFlags = stages,
        .offset = range.offset,
        .size = range.size,
    });
  }

  VkPipelineLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_info.setLayoutCount = static_cast<uint32_t>(vk_sets_layouts.size());
  layout_info.pSetLayouts = vk_sets_layouts.data();
  layout_info.pushConstantRangeCount =
      static_cast<uint32_t>(vk_push_constant_ranges.size());
  layout_info.pPushConstantRanges = vk_push_constant_ranges.data();

  QUARK_VK_TRY(vkCreatePipelineLayout(device_.device, &layout_info,
                                      /*pAllocator=*/nullptr, &handle_));

  QUARK_OK();
}

void PipelineLayoutBackend::destroy() noexcept {
  if (device_.device != VK_NULL_HANDLE && handle_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_.device, handle_, /*pAllocator=*/nullptr);
  }

  handle_ = VK_NULL_HANDLE;
  device_ = {};
}

} // namespace quark::vk
