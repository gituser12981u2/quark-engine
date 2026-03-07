#include <cstdint>
#include <quark/vk/device/device_view.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/frame/details/frame_sync.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

util::Status FrameSync::create(const CreateInfo &ci) {
  destroy();

  QUARK_TRY_VALIDATE(validate(ci.device));

  QUARK_ENSURE(
      ci.frames_in_flight > 0,
      QUARK_ERR(util::Errc::InvalidArg, "frames_in_flight must be > 0"));

  vk_device_ = ci.device.device;
  alloc_ = ci.allocator;
  frames_idle_on_create_ = ci.frames_idle_on_create;

  QUARK_TRY_STATUS(create_per_frame_(ci.frames_in_flight));

  QUARK_OK();
}

void FrameSync::destroy() noexcept {
  destroy_per_frame_();

  vk_device_ = VK_NULL_HANDLE;
  alloc_ = nullptr;
  frames_idle_on_create_ = true;
}

util::Status FrameSync::resize(uint32_t new_frames_in_flight) {
  QUARK_ENSURE(vk_device_ != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "not created"));
  QUARK_ENSURE(
      new_frames_in_flight > 0,
      QUARK_ERR(util::Errc::InvalidArg, "frames_in_flight must be > 0"));

  destroy_per_frame_();
  return create_per_frame_(new_frames_in_flight);
}

util::Status FrameSync::create_per_frame_(uint32_t count) {
  frames_.assign(count, PerFrame{});

  if (frames_idle_on_create_) {
    for (auto &frame : frames_) {
      frame.in_flight_value = 0; // idle sentinel
    }
  }

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  for (uint32_t i = 0; i < count; ++i) {
    QUARK_VK_TRY(vkCreateSemaphore(vk_device_, &semaphore_info, alloc_,
                                   &frames_[i].image_available));
    QUARK_VK_TRY(vkCreateSemaphore(vk_device_, &semaphore_info, alloc_,
                                   &frames_[i].render_finished));
  }

  QUARK_OK();
}

void FrameSync::destroy_per_frame_() noexcept {
  if (vk_device_ == VK_NULL_HANDLE) {
    frames_.clear();
    return;
  }

  for (auto &frame : frames_) {
    if (frame.image_available != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk_device_, frame.image_available, alloc_);
      frame.image_available = VK_NULL_HANDLE;
    }

    if (frame.render_finished != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk_device_, frame.render_finished, alloc_);
      frame.render_finished = VK_NULL_HANDLE;
    }

    frame.in_flight_value = 0;
  }

  frames_.clear();
}

} // namespace quark::vk::details
