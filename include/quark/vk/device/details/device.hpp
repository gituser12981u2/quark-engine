#pragma once

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

class Device final {
public:
  struct CreateInfo {
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    std::vector<const char *> required_extensions;
  };

  Device() = default;
  ~Device() { destroy(); }

  QUARK_MOVE_ONLY(Device);

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return device_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkPhysicalDevice vk_physical_device() const noexcept {
    return physical_device_;
  }

  [[nodiscard]] VkDevice vk_device() const noexcept { return device_; }

  [[nodiscard]] VkQueue graphics_queue() const noexcept {
    return graphics_queue_;
  }

  [[nodiscard]] VkQueue present_queue() const noexcept {
    return present_queue_;
  }

  [[nodiscard]] uint32_t graphics_queue_family_index() const noexcept {
    return graphics_queue_family_index_;
  }

  [[nodiscard]] uint32_t present_queue_family_index() const noexcept {
    return present_queue_family_index_;
  }

private:
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;
  uint32_t graphics_queue_family_index_ = 0;
  uint32_t present_queue_family_index_ = 0;
};

} // namespace quark::vk::details
