#pragma once

#include "quark/rhi/pipeline/pipeline_layout_desc.hpp"
#include "quark/vk/device/device_view.hpp"

#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include <vulkan/vulkan_core.h>

namespace quark::vk {

class PipelineLayoutBackend final {
public:
  struct CreateInfo {
    DeviceView device{};
    const rhi::PipelineLayoutDesc *desc{nullptr};
  };

  PipelineLayoutBackend() = default;
  ~PipelineLayoutBackend() { destroy(); }

  QUARK_MOVE_ONLY(PipelineLayoutBackend);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] DeviceView device() const noexcept { return device_; }

  [[nodiscard]] VkPipelineLayout vk_handle() const noexcept { return handle_; }

private:
  DeviceView device_{};
  VkPipelineLayout handle_{VK_NULL_HANDLE};
};

} // namespace quark::vk
