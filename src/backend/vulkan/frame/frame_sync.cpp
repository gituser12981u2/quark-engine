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

  QUARK_TRY_STATUS(create_timeline_());
  QUARK_TRY_STATUS(create_per_frame_(ci.frames_in_flight));

  QUARK_OK();
}

void FrameSync::destroy() noexcept {
  destroy_per_frame_();
  destroy_timeline_();

  vk_device_ = VK_NULL_HANDLE;
  alloc_ = nullptr;
  frames_idle_on_create_ = true;
  next_value_ = 1;
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

util::Status FrameSync::create_timeline_() {
  QUARK_ENSURE(vk_device_ != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "device is null"));

  VkSemaphoreTypeCreateInfo type_info{};
  type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  type_info.initialValue = 0;

  VkSemaphoreCreateInfo sem_ci{};
  sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  sem_ci.pNext = &type_info;

  QUARK_VK_TRY(vkCreateSemaphore(vk_device_, &sem_ci, alloc_, &timeline_));

  next_value_ = 1;
  QUARK_OK();
}

void FrameSync::destroy_timeline_() noexcept {
  if (vk_device_ != VK_NULL_HANDLE && timeline_ != VK_NULL_HANDLE) {
    vkDestroySemaphore(vk_device_, timeline_, alloc_);
  }

  timeline_ = VK_NULL_HANDLE;
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

util::Status FrameSync::wait_frame(uint32_t frame, uint64_t timeout_ns) const {
  QUARK_ENSURE(vk_device_ != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "not created"));
  QUARK_ENSURE(
      timeline_ != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidState, "timeline semaphore is null"));
  QUARK_ENSURE(frame < frames_.size(),
               QUARK_ERR(util::Errc::InvalidArg, "frame out of range"));

  const uint64_t v = frames_[frame].in_flight_value;
  if (v == 0) {
    QUARK_OK();
  }

  VkSemaphoreWaitInfo wait_info{};
  wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
  wait_info.flags = 0;
  wait_info.semaphoreCount = 1;
  wait_info.pSemaphores = &timeline_;
  wait_info.pValues = &v;

  // TODO: make better pattern for different VK result pattern matching
  const VkResult r = vkWaitSemaphores(vk_device_, &wait_info, timeout_ns);
  if (r == VK_TIMEOUT) {
    QUARK_FAIL(QUARK_ERR(util::Errc::Timeout, "vkWaitSemaphores timeout"));
  }
  QUARK_ENSURE(r == VK_SUCCESS, ::quark::vk::vk_error(r, "vkWaitSemaphores"));

  QUARK_OK();
}

} // namespace quark::vk::details
