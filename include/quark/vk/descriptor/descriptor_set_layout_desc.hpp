#pragma once

#include <span>
#include <vulkan/vulkan_core.h>

struct DescriptorSetLayoutDesc {
  std::span<const VkDescriptorSetLayoutBinding> bindings;
  VkDescriptorSetLayoutCreateFlags flags{};
};
