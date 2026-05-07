#pragma once

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vulkan/vulkan.h>

namespace quark::vk {

class DebugMessenger final {
public:
  struct CreateInfo {
    VkDebugUtilsMessageSeverityFlagsEXT severity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    VkDebugUtilsMessageTypeFlagsEXT types =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    PFN_vkDebugUtilsMessengerCallbackEXT callback = nullptr;
    void *user_data = nullptr;
  };

  DebugMessenger() = default;
  ~DebugMessenger() { destroy(); }

  QUARK_MOVE_ONLY(DebugMessenger);

  util::Status create(VkInstance instance, const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return messenger_ != VK_NULL_HANDLE;
  }
  [[nodiscard]] VkDebugUtilsMessengerEXT handle() const noexcept {
    return messenger_;
  }

private:
  VkInstance instance_ = VK_NULL_HANDLE; // non-owning

  VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
};

} // namespace quark::vk
