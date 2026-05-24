#pragma once

#include <cstdint>
#include <quark/utils/result.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

struct DeviceView {
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkQueue graphics_queue = VK_NULL_HANDLE;
  uint32_t graphics_queue_family_index = 0;
};

[[nodiscard]] util::Status validate(const DeviceView &view) noexcept;

} // namespace quark::vk
