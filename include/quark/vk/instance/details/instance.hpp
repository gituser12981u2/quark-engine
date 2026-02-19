#pragma once

#include "debug_messenger.hpp"

#include <cstdint>
#include <quark/utils/diagnostic.hpp>
#include <quark/utils/raii.hpp>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class Instance final {
public:
  struct CreateInfo {
    std::string_view app_name = "quark-engine";
    uint32_t app_version = VK_MAKE_API_VERSION(0, 0, 1, 0);
    std::string_view engine_name = "quark";
    uint32_t engine_version = VK_MAKE_API_VERSION(0, 0, 1, 0);
    uint32_t api_version = VK_API_VERSION_1_3;

    std::vector<const char *> extensions;

    // Validation layers
    std::vector<const char *> layers;

    bool enable_debug_messenger = false;
    DebugMessenger::CreateInfo debug{};
  };

  Instance() = default;
  ~Instance() { destroy(); }

  QUARK_MOVE_ONLY(Instance);

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return instance_ != VK_NULL_HANDLE;
  }
  [[nodiscard]] VkInstance handle() const noexcept { return instance_; }
  [[nodiscard]] const DebugMessenger &debug_messenger() const noexcept {
    return debug_messenger_;
  }

private:
  /**
   * @brief Build the final instance extension list and create flags.
   *
   * Behavior:
   * - Starts from {@code ci.extensions}
   * - On Apple/MoltenVK< requires {@code VK_KHR_portability_enumeration} and
   * sets {@code VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR}.
   * - If {@code ci.enable_debug_messenger} is true, required {@code
   * VK_EXT_debug_utils}. If unavailable, returns {@code Errc::Unsupported}.
   *
   * @param ci User-provided instance create info.
   * @param out_exts Ouput extension name list to pass to vkCreateInstance.
   * @param out_flags Output VkInstanceCreateFlags to pass to vkCreateInstance.
   * @param out_enable_debug_utils Whehter debug utils is enabled.
   */
  [[nodiscard]] static util::Status build_instance_extensions_(
      const CreateInfo &ci, std::vector<const char *> &out_exts,
      VkInstanceCreateFlags &out_flags, bool &out_enable_debug_utils);

  VkInstance instance_ = VK_NULL_HANDLE;
  DebugMessenger debug_messenger_;
};

} // namespace quark::vk
