#pragma once

#include "quark/rhi/pipeline/pipeline_layout_desc.hpp"
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/device_view.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class PipelineLayoutBackend final {
public:
  struct CreateInfo {
    DeviceView device{};

    const rhi::PipelineLayoutDesc *desc{nullptr};

    // uint32_t set_layout_count{0};
    // const VkDescriptorSetLayout *set_layouts{nullptr};
    //
    // uint32_t push_constant_range_count{0};
    // const VkPushConstantRange *push_constant_ranges{nullptr};
    //
    // const VkAllocationCallbacks *allocator{nullptr};
  };

  PipelineLayoutBackend() = default;
  ~PipelineLayoutBackend() { destroy(); }

  QUARK_MOVE_ONLY(PipelineLayoutBackend);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPipelineLayout handle() const noexcept { return handle_; }

private:
  DeviceView device_{};
  VkPipelineLayout handle_{VK_NULL_HANDLE};
};

} // namespace quark::vk
