#include <cstdint>
#include <quark/vk/device/device_view.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/sync/gpu_timeline.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

util::Status GpuTimeline::create(const CreateInfo &ci) {
  destroy();

  QUARK_TRY_VALIDATE(validate(ci.device));
  QUARK_ENSURE(ci.initial_value < UINT64_MAX,
               QUARK_ERR(util::Errc::InvalidArg, "initial_value is invalid"));
  QUARK_ENSURE(
      ci.first_signal_value == 0,
      QUARK_ERR(util::Errc::InvalidArg, "first_signal_value must be = 0"));

  vk_device_ = ci.device.device;
  alloc_ = ci.allocator;

  VkSemaphoreTypeCreateInfo type_info{};
  type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
  type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
  type_info.initialValue = ci.initial_value;

  VkSemaphoreCreateInfo sem_ci{};
  sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  sem_ci.pNext = &type_info;

  QUARK_VK_TRY(vkCreateSemaphore(vk_device_, &sem_ci, alloc_, &timeline_));

  next_value_ = ci.first_signal_value;
  QUARK_OK();
}

void GpuTimeline::destroy() noexcept {
  if (vk_device_ != VK_NULL_HANDLE && timeline_ != VK_NULL_HANDLE) {
    vkDestroySemaphore(vk_device_, timeline_, alloc_);
  }

  timeline_ = VK_NULL_HANDLE;
  vk_device_ = VK_NULL_HANDLE;
  alloc_ = nullptr;
  next_value_ = 1;
}

util::Result<uint64_t> GpuTimeline::completed_value() const noexcept {
  QUARK_ENSURE(vk_device_ != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "not created"));
  QUARK_ENSURE(
      timeline_ != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidState, "timeline semaphore is null"));

  uint64_t v = 0;
  QUARK_VK_TRY(vkGetSemaphoreCounterValue(vk_device_, timeline_, &v));
  return v;
}

util::Status GpuTimeline::wait(uint64_t value, uint64_t timeout_ns) const {
  QUARK_ENSURE(vk_device_ != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "not created"));
  QUARK_ENSURE(
      timeline_ != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidState, "timeline semaphore is null"));

  if (value == 0) {
    QUARK_OK();
  }

  VkSemaphoreWaitInfo wait_info{};
  wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
  wait_info.semaphoreCount = 1;
  wait_info.pSemaphores = &timeline_;
  wait_info.pValues = &value;

  // TODO: make better pattern for different VK result pattern matching
  const VkResult r = vkWaitSemaphores(vk_device_, &wait_info, timeout_ns);
  if (r == VK_TIMEOUT) {
    QUARK_FAIL(QUARK_ERR(util::Errc::Timeout, "vkWaitSemaphores timeout"));
  }
  QUARK_ENSURE(r == VK_SUCCESS, ::quark::vk::vk_error(r, "vkWaitSemaphores"));

  QUARK_OK();
}

} // namespace quark::vk
