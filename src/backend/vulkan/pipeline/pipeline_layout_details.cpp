#include <quark/utils/error_types.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/pipeline/details/pipeline_layout.hpp>

namespace quark::vk::details {

util::Status PipelineLayout::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate(ci.device));

  destroy();

  device_ = ci.device;
  allocator_ = ci.allocator;

  VkPipelineLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_info.setLayoutCount = ci.set_layout_count;
  layout_info.pSetLayouts = ci.set_layouts;
  layout_info.pushConstantRangeCount = ci.push_constant_range_count;
  layout_info.pPushConstantRanges = ci.push_constant_ranges;

  QUARK_VK_TRY(vkCreatePipelineLayout(device_.device, &layout_info, allocator_,
                                      &handle_));

  QUARK_OK();
}

void PipelineLayout::destroy() noexcept {
  if (device_.device != VK_NULL_HANDLE && handle_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device_.device, handle_, allocator_);
  }

  handle_ = VK_NULL_HANDLE;
  device_ = {};
  allocator_ = nullptr;
}

} // namespace quark::vk::details
