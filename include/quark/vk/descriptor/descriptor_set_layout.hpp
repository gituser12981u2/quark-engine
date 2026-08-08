#pragma once

#include "quark/utils/raii.hpp"
#include "quark/vk/descriptor/descriptor_set_layout_desc.hpp"
#include "quark/vk/device/device_view.hpp"

#include <vulkan/vulkan_core.h>

namespace quark::vk {

class DescriptorSetLayout final {
public:
  struct CreateInfo {
    DeviceView device{};
    const VkAllocationCallbacks *allocator{nullptr};
    const DescriptorSetLayoutDesc *desc{nullptr};
  };

  DescriptorSetLayout() = default;
  ~DescriptorSetLayout() { destroy(); }

  QUARK_MOVE_ONLY(DescriptorSetLayout);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkDescriptorSetLayout handle() const noexcept {
    return handle_;
  }

private:
  DeviceView device_{};
  VkDescriptorSetLayout handle_{VK_NULL_HANDLE};
  const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace quark::vk
