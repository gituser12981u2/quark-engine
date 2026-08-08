#include "quark/rhi/descriptor/descriptor_set_layout.hpp"
#include "quark/engine/registry/backend_registry.hpp"
#include "quark/rhi/descriptor/details/descriptor_set_layout_handle.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/vk/descriptor/descriptor_set_layout_backend.hpp"

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

namespace {
using DescriptorSetLayoutRegistry =
    engine::BackendRegistry<rhi::details::DescriptorSetLayoutHandle,
                            vk::DescriptorSetLayoutBackend>;
}

DescriptorSetLayoutRegistry &descriptor_set_layout_registry() noexcept {
  static DescriptorSetLayoutRegistry registry;
  return registry;
}

} // namespace quark::vk::details

namespace quark::rhi {

DescriptorSetLayout::DescriptorSetLayout() noexcept = default;

DescriptorSetLayout::~DescriptorSetLayout() { destroy(); }

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) noexcept
    : device_(std::exchange(other.device_, DeviceView{})),
      retire_queue_(std::exchange(other.retire_queue_, nullptr)),
      handle_(
          std::exchange(other.handle_, details::DescriptorSetLayoutHandle{})) {}

DescriptorSetLayout &
DescriptorSetLayout::operator=(DescriptorSetLayout &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  destroy();

  device_ = std::exchange(other.device_, DeviceView{});
  retire_queue_ = std::exchange(other.retire_queue_, nullptr);
  handle_ = std::exchange(other.handle_, details::DescriptorSetLayoutHandle{});

  return *this;
}

util::Status DescriptorSetLayout::create(const CreateInfo &ci) {
  QUARK_ENSURE(ci.device.valid(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "descriptor set layout device is invalid"));

  QUARK_ENSURE(
      ci.desc != nullptr,
      QUARK_ERR(util::Errc::InvalidArg, "descriptor set layout desc is null"));

  const vk::DescriptorSetLayoutBackend::CreateInfo backend_ci{
      .device = ci.device.backend(), .desc = ci.desc};

  details::DescriptorSetLayoutHandle new_handle{};

  QUARK_TRY_ASSIGN(
      new_handle,
      vk::details::descriptor_set_layout_registry().create_backend(backend_ci));

  destroy();

  device_ = ci.device;
  retire_queue_ = ci.retire_queue;
  handle_ = new_handle;

  QUARK_OK();
}

void DescriptorSetLayout::destroy() noexcept {
  if (handle_.valid()) {
    vk::details::descriptor_set_layout_registry().destroy_backend(handle_);
  }

  device_ = {};
  retire_queue_ = nullptr;
  handle_ = {};
}

void DescriptorSetLayout::retire(uint64_t retire_at) noexcept {
  if (handle_.valid()) {
    vk::details::descriptor_set_layout_registry().retire(handle_, retire_queue_,
                                                         retire_at);
  }

  device_ = {};
  retire_queue_ = nullptr;
  handle_ = {};
}

bool DescriptorSetLayout::valid() const noexcept {
  if (!device_.valid() || !handle_.valid()) {
    return false;
  }

  const vk::DescriptorSetLayoutBackend *backend =
      vk::details::descriptor_set_layout_registry().backend(handle_);

  return backend != nullptr && backend->valid();
}

NativeBackendRef DescriptorSetLayout::native_backend() const noexcept {
  if (!valid()) {
    return {};
  }

  const vk::DescriptorSetLayoutBackend *backend =
      vk::details::descriptor_set_layout_registry().backend(handle_);

  return NativeBackendRef{
      .ptr = backend,
  };
}

} // namespace quark::rhi
