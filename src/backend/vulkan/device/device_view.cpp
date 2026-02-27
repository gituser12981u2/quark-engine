#include <quark/utils/diagnostic.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/device/device_view.hpp>

namespace quark::vk {

util::Status validate(const DeviceView &view) noexcept {
  QUARK_ENSURE(view.device != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "VkDevice is null"));
  QUARK_ENSURE(view.physical_device != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "VkPhysicalDevice is null"));
  QUARK_ENSURE(view.graphics_queue != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidState, "graphics queue is null"));
  return {};
}

} // namespace quark::vk
