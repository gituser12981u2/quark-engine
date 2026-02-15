#include <bit>
#include <quark/vk/instance/details/debug_messenger.hpp>
#include <stdexcept>
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

void DebugMessenger::create(VkInstance instance, const CreateInfo &ci) {
  destroy();

  if (instance == VK_NULL_HANDLE) {
    throw std::runtime_error("DebugMessenger::create: Instance is null");
  }

  if (ci.callback == nullptr) {
    throw std::runtime_error("DebugMessenger::create: callback is null");
  }

  const auto create_fn = load_create_fn(instance);
  if (create_fn == nullptr) {
    throw std::runtime_error("vkCreateDebugUtilsMessengerEXT not found");
  }

  VkDebugUtilsMessengerCreateInfoEXT info{};
  info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  info.messageSeverity = ci.severity;
  info.messageType = ci.types;
  info.pfnUserCallback = ci.callback;
  info.pUserData = ci.user_data;

  VkDebugUtilsMessengerEXT out = VK_NULL_HANDLE;
  const VkResult r = create_fn(instance, &info, nullptr, &out);
  if (r != VK_SUCCESS) {
    throw std::runtime_error("vkCreateDebugUtilsMessengerEXT failed");
  }

  instance_ = instance;
  messenger_ = out;
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
