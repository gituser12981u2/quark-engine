#pragma once

#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/descriptor/details/descriptor_set_layout_handle.hpp"
#include "quark/vk/descriptor/details/descriptor_set_layout_registry.hpp"
#include "quark/vk/device/device_view.hpp"
#include "quark/vk/pipeline/details/pipeline_layout_handle.hpp"
#include "quark/vk/pipeline/details/pipeline_layout_registry.hpp"
#include <span>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class RetirementQueue;

struct PipelineLayoutDesc {
  std::span<const details::DescriptorSetLayoutHandle> descriptor_set_layouts;
  std::span<const VkPushConstantRange> push_constant_ranges;
};

class PipelineLayout final {
public:
  using Desc = PipelineLayoutDesc;

  struct CreateInfo {
    DeviceView device{};
    const details::DescriptorSetLayoutRegistry *descriptor_set_layouts{nullptr};
    RetirementQueue *retire_queue{nullptr};
    const VkAllocationCallbacks *allocator{nullptr};

    const Desc *desc{nullptr};
  };

  PipelineLayout() = default;
  ~PipelineLayout() { destroy(); }

  QUARK_MOVE_ONLY(PipelineLayout);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_.valid() && backend_ != nullptr && backend_->valid();
  }

  [[nodiscard]] details::PipelineLayoutHandle handle() const noexcept {
    return handle_;
  }

  [[nodiscard]] VkPipelineLayout vk_handle() const noexcept {
    return valid() ? backend_->handle() : VK_NULL_HANDLE;
  }

private:
  details::PipelineLayoutRegistry registry_;
  details::PipelineLayoutHandle handle_{};
  const details::PipelineLayout *backend_{nullptr};
};

} // namespace quark::vk
