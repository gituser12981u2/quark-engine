#pragma once

#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"

namespace quark::vk::details {

class Pipeline final {
public:
  struct CreateInfo {
    DeviceView device{};

    VkPipelineCache cache{VK_NULL_HANDLE};

    const VkGraphicsPipelineCreateInfo *graphics_info{nullptr};

    const VkAllocationCallbacks *allocator{nullptr};
  };

  Pipeline() = default;
  ~Pipeline() { destroy(); }

  QUARK_MOVE_ONLY(Pipeline);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPipeline handle() const noexcept { return handle_; }

private:
  DeviceView device_{};
  VkPipeline handle_{VK_NULL_HANDLE};
  const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace quark::vk::details
