#include "quark/utils/diagnostic.hpp"
#include <cstdint>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/frame/details/frame_cmd.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

util::Status FrameCmd::create(const CreateInfo &ci) {
  destroy();

  QUARK_TRY_VALIDATE(validate(ci.device));

  vk_device_ = ci.device.device;
  graphics_qfi_ = ci.device.graphics_queue_family_index;
  alloc_ = ci.allocator;

  QUARK_ENSURE(vk_device_ != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg, "VkDevice is null"));

  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = ci.pool_flags;
  pool_info.queueFamilyIndex = graphics_qfi_;

  QUARK_VK_TRY(vkCreateCommandPool(vk_device_, &pool_info, alloc_, &pool_));

  QUARK_TRY_STATUS(resize(ci.buffer_count));
  return {};
}

void FrameCmd::free_buffers_() noexcept {
  if (!buffers_.empty() && vk_device_ != VK_NULL_HANDLE &&
      pool_ != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(vk_device_, pool_,
                         static_cast<uint32_t>(buffers_.size()),
                         buffers_.data());
    buffers_.clear();
  }
}

util::Status FrameCmd::resize(uint32_t new_count) {
  QUARK_ENSURE(vk_device_ != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "device is null"));
  QUARK_ENSURE(pool_ != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "pool is null"));

  if (new_count == static_cast<uint32_t>(buffers_.size())) {
    return {};
  }

  free_buffers_();

  buffers_.assign(new_count, VK_NULL_HANDLE);

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = new_count;

  QUARK_VK_TRY(
      vkAllocateCommandBuffers(vk_device_, &alloc_info, buffers_.data()));

  // TODO: remove this pattern
  return {};
}

void FrameCmd::destroy() noexcept {
  free_buffers_();

  if (vk_device_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(vk_device_, pool_, alloc_);
  }

  pool_ = VK_NULL_HANDLE;
  vk_device_ = VK_NULL_HANDLE;
  graphics_qfi_ = 0;
  alloc_ = nullptr;
  buffers_.clear();
}

} // namespace quark::vk::details
