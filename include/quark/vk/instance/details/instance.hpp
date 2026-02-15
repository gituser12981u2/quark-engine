#pragma once

#include "debug_messenger.hpp"

#include <cstdint>
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
  explicit Instance(const CreateInfo &ci) { create(ci); }
  ~Instance() { destroy(); }

  QUARK_MOVE_ONLY(Instance);

  void create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return instance_ != VK_NULL_HANDLE;
  }
  [[nodiscard]] VkInstance handle() const noexcept { return instance_; }
  [[nodiscard]] const DebugMessenger &debug_messenger() const noexcept {
    return debug_messenger_;
  }

private:
  VkInstance instance_ = VK_NULL_HANDLE;
  DebugMessenger debug_messenger_;
};

} // namespace quark::vk
