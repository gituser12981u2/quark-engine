#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/pipeline/details/pipeline.hpp>

namespace quark::vk::details {

util::Status Pipeline::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate(ci.device));

  QUARK_ENSURE(ci.graphics_info != nullptr,
               QUARK_ERR(util::Errc::InvalidArg,
                         "graphics pipeline create info is null"));

  destroy();

  device_ = ci.device;

  const VkResult result =
      vkCreateGraphicsPipelines(device_.device, ci.cache, /*createInfoCount=*/1,
                                ci.graphics_info, allocator_, &handle_);

  if (result != VK_SUCCESS) {
    destroy();
    QUARK_FAIL(::quark::vk::vk_error(result, "vkCreateGraphicsPipelines"));
  }

  QUARK_OK();
}

void Pipeline::destroy() noexcept {
  if (device_.device != VK_NULL_HANDLE && handle_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device_.device, handle_, allocator_);
  }

  handle_ = VK_NULL_HANDLE;
  device_ = {};
}

} // namespace quark::vk::details
