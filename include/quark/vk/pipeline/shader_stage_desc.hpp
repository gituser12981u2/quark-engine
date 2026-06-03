#pragma once

#include <vulkan/vulkan_core.h>

namespace quark::vk {

struct ShaderStageDesc {
  // ShaderHandle shader{};
  VkShaderModule module{VK_NULL_HANDLE};
  VkShaderStageFlagBits stage{};
  const char *entry_point{"main"};
};

} // namespace quark::vk
