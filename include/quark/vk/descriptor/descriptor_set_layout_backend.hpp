#pragma once

#include "quark/rhi/descriptor/descriptor_set_layout_desc.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class DescriptorSetLayoutBackend final {
public:
  struct CreateInfo {
    DeviceView device{};
    const rhi::DescriptorSetLayoutDesc *desc{nullptr};
    // VkDescriptorSetLayoutCreateFlags flags{};
    // const VkAllocationCallbacks *allocator{nullptr};
  };

  DescriptorSetLayoutBackend() = default;
  ~DescriptorSetLayoutBackend() { destroy(); }

  QUARK_MOVE_ONLY(DescriptorSetLayoutBackend);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return handle_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkDescriptorSetLayout vk_handle() const noexcept {
    return handle_;
  }

private:
  DeviceView device_{};
  VkDescriptorSetLayout handle_{VK_NULL_HANDLE};
  // const VkAllocationCallbacks *allocator_{nullptr};
};

} // namespace quark::vk
