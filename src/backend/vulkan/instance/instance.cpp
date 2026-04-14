#include "quark/utils/details/diagnostic_details.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"
#include "quark/vk/instance/details/debug_messenger.hpp"
#include "quark/vk/vk_error.hpp"
#include <cstdint>
#include <cstring>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <source_location>
#include <string_view>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL default_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *user_data) {

  const bool is_warn =
      (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0;
  const bool is_err =
      (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0;
  if (!is_warn && !is_err) {
    return VK_FALSE;
  }

  const util::Severity sev =
      is_err ? util::Severity::Error : util::Severity::Warning;

  const char *id_name = (data != nullptr && data->pMessageIdName != nullptr)
                            ? data->pMessageIdName
                            : "unknown-id";
  const int32_t id_num = (data != nullptr) ? data->messageIdNumber : 0;
  const char *msg = (data != nullptr && data->pMessage != nullptr)
                        ? data->pMessage
                        : "(null)";

  (void)type;
  (void)user_data;

  util::report(util::details::make_event(sev, /*module=*/"vk.validation",
                                         std::source_location::current(),
                                         "[{}:{}] {}", id_name, id_num, msg));
  return VK_FALSE;
}

[[nodiscard]] util::Result<std::vector<VkExtensionProperties>>
enumerate_instance_extensions() {
  while (true) {
    uint32_t extension_count = 0;
    QUARK_VK_TRY(vkEnumerateInstanceExtensionProperties(
        /*pLayerName=*/nullptr, &extension_count, /*pProperties=*/nullptr));

    std::vector<VkExtensionProperties> props(extension_count);

    uint32_t written = extension_count;
    const VkResult r =
        vkEnumerateInstanceExtensionProperties(nullptr, &written, props.data());

    if (r == VK_SUCCESS) {
      props.resize(written);
      return props;
    }

    if (r != VK_INCOMPLETE) {
      return util::unexpected(::quark::vk::vk_error(
          r, "vkEnumerateInstanceExtensionProperties(data)",
          std::source_location::current()));
    }
  }
}

[[nodiscard]] util::Result<bool>
has_instance_extension(const char *extension_name) {
  QUARK_ENSURE(extension_name != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "extension name is null"));
  QUARK_ENSURE(extension_name[0] != '\0',
               QUARK_ERR(util::Errc::InvalidArg, "extension name is empty"));

  std::vector<VkExtensionProperties> props;
  QUARK_TRY_ASSIGN(props, enumerate_instance_extensions());

  for (const auto &p : props) {
    if (std::strcmp(p.extensionName, extension_name) == 0) {
      return true;
    }
  }

  return false;
}

[[nodiscard]] util::Result<bool>
enable_extension_if_available(std::vector<const char *> &exts, const char *name,
                              std::string_view log_name,
                              bool required = false) {
  bool has = false;
  QUARK_TRY_ASSIGN(has, has_instance_extension(name));

  if (!has) {
    if (required) {
      return util::unexpected(QUARK_ERR(
          util::Errc::Unsupported, "{} requested by not supported", log_name));
    }

    QUARK_LOG_WARN("{} requested but unavailable; disabling", log_name);

    return false;
  }

  exts.push_back(name);
  QUARK_LOG_INFO("{}: enabled", log_name);
  return true;
}

} // namespace

util::Status Instance::build_instance_extensions_(
    const CreateInfo &ci, std::vector<const char *> &out_exts,
    VkInstanceCreateFlags &out_flags, bool &out_enable_debug_utils) {
  out_exts = ci.extensions;
  out_flags = 0;
  out_enable_debug_utils = false;

  bool enable_portability = false;
#ifdef __APPLE__
  constexpr bool portability_required = true;
#else
  constexpr bool portability_required = false;
#endif
  QUARK_LOG_INFO("portability required: '{}'", portability_required);

  QUARK_TRY_ASSIGN(enable_portability,
                   enable_extension_if_available(
                       out_exts, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
                       "portability enumeration", portability_required));

  if (enable_portability) {
    out_flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }

  QUARK_LOG_INFO("debug messenger is requested: '{}'",
                 ci.enable_debug_messenger);

  if (ci.enable_debug_messenger) {
    QUARK_TRY_ASSIGN(out_enable_debug_utils,
                     enable_extension_if_available(
                         out_exts, VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                         "debug utils", /*required=*/true));
  }

  return {};
}

util::Status Instance::create(const CreateInfo &ci) {
  destroy();

  QUARK_ENSURE(!ci.app_name.empty(), QUARK_ERR(util::Errc::InvalidArg,
                                               "CreateInfo.app_name is empty"));
  QUARK_ENSURE(
      !ci.engine_name.empty(),
      QUARK_ERR(util::Errc::InvalidArg, "CreateInfo.engine_name is empty"));

  QUARK_LOG_INFO("app='{}' engine='{}' api={}.{}.{}", ci.app_name,
                 ci.engine_name, VK_VERSION_MAJOR(ci.api_version),
                 VK_VERSION_MINOR(ci.api_version),
                 VK_VERSION_PATCH(ci.api_version));

  uint32_t loader_ver = VK_API_VERSION_1_0;
#ifdef VK_VERSION_1_1
  vkEnumerateInstanceVersion(&loader_ver);
#endif
  QUARK_LOG_INFO("loader api version: {}.{}.{}", VK_VERSION_MAJOR(loader_ver),
                 VK_VERSION_MINOR(loader_ver), VK_VERSION_PATCH(loader_ver));

  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = ci.app_name.data();
  app.applicationVersion = ci.app_version;
  app.pEngineName = ci.engine_name.data();
  app.engineVersion = ci.engine_version;
  app.apiVersion = ci.api_version;

  std::vector<const char *> exts = ci.extensions;
  VkInstanceCreateFlags flags = 0;

  bool enable_debug_utils = false;
  QUARK_TRY_STATUS(
      build_instance_extensions_(ci, exts, flags, enable_debug_utils));

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
  if (ci.enable_debug_messenger && enable_debug_utils) {
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
  QUARK_VK_TRY(vkCreateInstance(&create, /*pAllocator=*/nullptr, &instance));
  instance_ = instance;

  if (ci.enable_debug_messenger && enable_debug_utils) {
    QUARK_TRY_STATUS(debug_messenger_.create(instance_, dbg));
  }

  return {};
}

void Instance::destroy() noexcept {
  debug_messenger_.destroy();

  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}

} // namespace quark::vk
