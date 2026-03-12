#pragma once

#include <quark/utils/diagnostic.hpp>
#include <quark/vk/vk_error.hpp>
#include <vulkan/vulkan.h>

/**
 * @def QUARK_VK_TRY(call)
 * @brief Executes a Vulkan call and converts VkResult failure into util::Error.
 *
 * Evaluates @p call exactly once. If it returns VK_SUCCESS, continues.
 * Otherwise returns util::unexpected(vk_error(result, "call")) from the current
 * function.
 *
 * Requirements:
 * - Current function returns util::Status or compatible util::Result<T>.
 * - vk_error(VkResult, const char*) must construct a util::Error with an API
 *   domain payload, message, and call-site location.
 *
 * Notes:
 * - The stringified expression (#call) is captured for diagnostics.
 */
#define QUARK_VK_TRY(call)                                                     \
  do {                                                                         \
    const VkResult _q_vk_r = (call);                                           \
    if (_q_vk_r != VK_SUCCESS) {                                               \
      return util::unexpected(::quark::vk::vk_error(_q_vk_r, #call));          \
    }                                                                          \
  } while (0)

/**
 * @def QUARK_VK_TRY_INCOMPLETE_OK(call)
 * @brief Like QUARK_VK_TRY, but treats VK_INCOMPLETE as success.
 *
 * Use for Vulkan enumeration patterns where VK_INCOMPLETE indicates that the
 * provided buffer was too small and the caller will retry.
 */
#define QUARK_VK_TRY_INCOMPLETE_OK(call)                                       \
  do {                                                                         \
    const VkResult _q_vk_r = (call);                                           \
    if (_q_vk_r != VK_SUCCESS && _q_vk_r != VK_INCOMPLETE) {                   \
      return util::unexpected(::quark::vk::vk_error(_q_vk_r, #call));          \
    }                                                                          \
  } while (0)
