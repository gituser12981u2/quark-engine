#include <cstdint>
#include <cstring>
#include <quark/vk/instance/details/debug_messenger.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

namespace quark::vk {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL default_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *) {
  if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ||
      (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
    // TODO: hook into logger
  }

  (void)data;
  return VK_FALSE;
}

[[nodiscard]] bool has_instance_extension(const char *extension_name) {
  uint32_t extension_count{0};
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

  std::vector<VkExtensionProperties> props(extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                         props.data());

  for (const auto &p : props) {
    if (std::strcmp(p.extensionName, extension_name) == 0) {
      return true;
    }
  }

  // return std::ranges::any_of(
  //     extensions, [extension_name](const VkExtensionProperties &extension) {
  //       return std::strcmp(extension.extensionName, extension_name) == 0;
  //     });

  return false;
}

} // namespace

void Instance::create(const CreateInfo &ci) {
  destroy();

  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = ci.app_name.data();
  app.applicationVersion = ci.app_version;
  app.pEngineName = ci.engine_name.data();
  app.engineVersion = ci.engine_version;
  app.apiVersion = ci.api_version;

  std::vector<const char *> exts = ci.extensions;

  VkInstanceCreateFlags flags = 0;
  constexpr const char *portability_enum = "VK_KHR_portability_enumeration";
  if (has_instance_extension(portability_enum)) {
    exts.push_back(portability_enum);
    flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }

  VkDebugUtilsMessengerCreateInfoEXT dbg_ci{};
  VkInstanceCreateInfo create{};
  create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create.flags = flags;
  create.pApplicationInfo = &app;
  create.enabledExtensionCount = static_cast<uint32_t>(exts.size());
  create.ppEnabledExtensionNames = exts.empty() ? nullptr : exts.data();
  create.enabledLayerCount = static_cast<uint32_t>(ci.layers.size());
  create.ppEnabledLayerNames = ci.layers.empty() ? nullptr : ci.layers.data();

  DebugMessenger::CreateInfo dbg = ci.debug;
  if (ci.enable_debug_messenger) {
    if (dbg.callback == nullptr) {
      dbg.callback = &default_debug_callback;
    }

    dbg_ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_ci.messageSeverity = dbg.severity;
    dbg_ci.messageType = dbg.types;
    dbg_ci.pfnUserCallback = dbg.callback;
    dbg_ci.pUserData = dbg.user_data;
    create.pNext = &dbg_ci;
  }

  VkInstance instance = VK_NULL_HANDLE;
  const VkResult r =
      vkCreateInstance(&create, /*pAllocator=*/nullptr, &instance);
  if (r != VK_SUCCESS) {
    throw std::runtime_error("vkCreateInstance failed");
  }

  instance_ = instance;

  if (ci.enable_debug_messenger) {
    debug_messenger_.create(instance_, dbg);
  }
}

void Instance::destroy() noexcept {
  debug_messenger_.destroy();

  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}

} // namespace quark::vk
