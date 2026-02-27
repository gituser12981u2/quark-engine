#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/device_view.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

class FrameCmd final {
public:
  struct CreateInfo {
    DeviceView device;
    uint32_t buffer_count = 0;
    VkCommandPoolCreateFlags pool_flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    const VkAllocationCallbacks *allocator = nullptr;
  };

  FrameCmd() = default;
  ~FrameCmd() { destroy(); }

  QUARK_MOVE_ONLY(FrameCmd);

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept {
    return vk_device_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkCommandPool pool() const noexcept { return pool_; }

  // One primary per swapchain image
  [[nodiscard]] VkCommandBuffer cmd(uint32_t index) const noexcept {
    return (index < buffers_.size()) ? buffers_[index] : VK_NULL_HANDLE;
  }

  [[nodiscard]] uint32_t count() const noexcept {
    return static_cast<uint32_t>(buffers_.size());
  }

  // Clear and reallocate buffers for new swapchain
  util::Status resize(uint32_t new_count);

private:
  void free_buffers_() noexcept;

  VkDevice vk_device_ = VK_NULL_HANDLE; // non-owning
  uint32_t graphics_qfi_ = 0;           // non-owning
  const VkAllocationCallbacks *alloc_ = nullptr;

  VkCommandPool pool_ = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> buffers_;
};

} // namespace quark::vk::details
