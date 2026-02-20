#include <quark/utils/diagnostic.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/instance/details/debug_messenger.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

namespace {

[[nodiscard]] PFN_vkCreateDebugUtilsMessengerEXT
load_create_fn(VkInstance instance) {
  return std::bit_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
}

[[nodiscard]] PFN_vkDestroyDebugUtilsMessengerEXT
load_destroy_fn(VkInstance instance) {
  return std::bit_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
}

} // namespace

util::Status DebugMessenger::create(VkInstance instance, const CreateInfo &ci) {
  destroy();
  QUARK_ENSURE(instance != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "Instance is VK_NULL_HANDLE"));
  QUARK_ENSURE(ci.callback != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "callback is null"));

  const auto create_fn = load_create_fn(instance);
  QUARK_ENSURE(create_fn != nullptr,
               QUARK_ERR(util::Errc::Unsupported,
                         "vkCreateDebugUtilsMessengerEXT not found"));

  VkDebugUtilsMessengerCreateInfoEXT info{};
  info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  info.messageSeverity = ci.severity;
  info.messageType = ci.types;
  info.pfnUserCallback = ci.callback;
  info.pUserData = ci.user_data;

  VkDebugUtilsMessengerEXT out = VK_NULL_HANDLE;
  QUARK_VK_TRY(create_fn(instance, &info, nullptr, &out));
  instance_ = instance;
  messenger_ = out;

  return {};
}

void DebugMessenger::destroy() noexcept {
  if (messenger_ == VK_NULL_HANDLE) {
    instance_ = VK_NULL_HANDLE;
    return;
  }

  if (instance_ != VK_NULL_HANDLE) {
    const auto destroy_fn = load_destroy_fn(instance_);
    if (destroy_fn != nullptr) {
      destroy_fn(instance_, messenger_, nullptr);
    }
  }

  messenger_ = VK_NULL_HANDLE;
  instance_ = VK_NULL_HANDLE;
}

} // namespace quark::vk
