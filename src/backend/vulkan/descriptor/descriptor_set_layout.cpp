#include "quark/vk/descriptor/descriptor_set_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"
#include "quark/vk/diagnostic_prelude.hpp"
#include <alloca.h>
#include <cstdint>
#include <limits>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status DescriptorSetLayout::create(const CreateInfo &ci) {
  QUARK_TRY_STATUS(validate(ci.device));

  QUARK_ENSURE(
      ci.desc != nullptr,
      QUARK_ERR(util::Errc::InvalidArg, "descriptor-set layout desc is null"));

  QUARK_ENSURE(ci.desc->bindings.size() <= std::numeric_limits<uint32_t>::max(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "descriptor-set layout has too many bindings"));

  destroy();

  device_ = ci.device;
  allocator_ = ci.allocator;

  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = static_cast<uint32_t>(ci.desc->bindings.size());
  layout_info.pBindings =
      ci.desc->bindings.empty() ? nullptr : ci.desc->bindings.data();

  QUARK_VK_TRY(vkCreateDescriptorSetLayout(device_.device, &layout_info,
                                           allocator_, &handle_));

  QUARK_OK();
}

void DescriptorSetLayout::destroy() noexcept {
  if (device_.device != VK_NULL_HANDLE && handle_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device_.device, handle_, allocator_);
  }

  handle_ = VK_NULL_HANDLE;
  device_ = {};
  allocator_ = nullptr;
}

} // namespace quark::vk
