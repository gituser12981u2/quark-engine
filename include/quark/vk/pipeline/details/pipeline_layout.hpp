#pragma once

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/device_view.hpp>

namespace quark::vk::details {

class PipelineLayout final {
public:
  struct CreateInfo {
    DeviceView device{};

    uint32_t set_layout_count{0};
    const VkDescriptorSetLayout *set_layouts{nullptr};

    uint32_t push_constant_range_count{0};
    const VkPushConstantRange *push_constant_ranges{nullptr};

    const VkAllocationCallbacks *allocator{nullptr};
  };

  PipelineLayout() = default;
  ~PipelineLayout() { destroy(); }

  QUARK_MOVE_ONLY(PipelineLayout);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPipelineLayout handle() const noexcept { return handle_; }

private:
  DeviceView device_{};
  VkPipelineLayout handle_{VK_NULL_HANDLE};
  const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace quark::vk::details
