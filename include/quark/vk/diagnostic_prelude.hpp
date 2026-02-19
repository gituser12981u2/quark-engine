#pragma once

#include <quark/utils/diagnostic.hpp>
#include <quark/vk/vk_error.hpp>
#include <vulkan/vulkan.h>

#define QUARK_VK_TRY(call)                                                     \
  do {                                                                         \
    const VkResult _q_vk_r = (call);                                           \
    if (_q_vk_r != VK_SUCCESS) {                                               \
      return util::unexpected(::quark::vk::vk_error(_q_vk_r, #call));          \
    }                                                                          \
  } while (0)

#define QUARK_VK_TRY_INCOMPLETE_OK(call)                                       \
  do {                                                                         \
    const VkResult _q_vk_r = (call);                                           \
    if (_q_vk_r != VK_SUCCESS && _q_vk_r != VK_INCOMPLETE) {                   \
      return util::unexpected(::quark::vk::vk_error(_q_vk_r, #call));          \
    }                                                                          \
  } while (0)
