#pragma once

#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/device/device_view.hpp"
#include <cstdint>
#include <span>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

struct DescriptorBindingDesc {
  uint32_t binding{};
  VkDescriptorType type{VK_DESCRIPTOR_TYPE_MAX_ENUM};
  uint32_t count{1};
  VkShaderStageFlags stages{};
  const VkSampler *immutable_samplers{nullptr};
};

struct DescriptorSetLayoutDesc {
  std::span<const DescriptorBindingDesc> bindings;
};

class DescriptorSetLayout final {
public:
  struct CreateInfo {
    DeviceView device{};
    DescriptorSetLayoutDesc desc{};
    VkDescriptorSetLayoutCreateFlags flags{};
    const VkAllocationCallbacks *allocator{nullptr};
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
