#include "quark/rhi/descriptor/descriptor_set_layout.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"

#include "quark/vk/descriptor/descriptor_set_layout_backend.hpp"

namespace quark::rhi {

util::Status DescriptorSetLayout::create(const CreateInfo &ci) {
  QUARK_ENSURE(
      ci.desc != nullptr,
      QUARK_ERR(util::Errc::InvalidArg, "descriptor set layout desc is null"));

  destroy();

  QUARK_TRY_STATUS(registry_.create({
      .retire_queue = ci.retire_queue,
  }));

  const vk::DescriptorSetLayoutBackend::CreateInfo backend_ci{
      .device = ci.device, .desc = ci.desc};

  QUARK_TRY_ASSIGN(handle_, registry_.create_backend(backend_ci));

  backend_ = registry_.backend(handle_);

  QUARK_OK();
}

void DescriptorSetLayout::destroy() noexcept {
  registry_.destroy();
  handle_ = {};
  backend_ = nullptr;
}

bool DescriptorSetLayout::valid() const noexcept {
  return handle_.valid() && backend_ != nullptr && backend_->valid();
}

VkDescriptorSetLayout DescriptorSetLayout::vk_handle() const noexcept {
  return valid() ? backend_->vk_handle() : VK_NULL_HANDLE;
}

} // namespace quark::rhi
