#include <quark/utils/diagnostic.hpp>
#include <quark/vk/allocator.hpp>
#include <quark/vk/vk_error.hpp>

namespace quark::vk {

util::Status Allocator::create(const Allocator::CreateInfo &ci) {
  destroy();

  QUARK_ENSURE(
      ci.instance != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidArg, "Allocator instance must be valid"));
  QUARK_ENSURE(ci.physical_device != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg,
                         "Allocator physical device must be valid"));
  QUARK_ENSURE(ci.device != VK_NULL_HANDLE,
               QUARK_ERR(util::Errc::InvalidArg,
                         "Allocator logical device must be valid"));

  const VmaAllocatorCreateInfo allocator_info{
      .flags = 0,
      .physicalDevice = ci.physical_device,
      .device = ci.device,
      .preferredLargeHeapBlockSize = 0,
      .pAllocationCallbacks = ci.allocation_callbacks,
      .pDeviceMemoryCallbacks = nullptr,
      .pHeapSizeLimit = nullptr,
      .pVulkanFunctions = nullptr,
      .instance = ci.instance,
      .vulkanApiVersion = ci.vulkan_api_version,
#if VMA_EXTERNAL_MEMORY
      .pTypeExternalMemoryHandleTypes = nullptr,
#endif
  };

  const VkResult result = vmaCreateAllocator(&allocator_info, &allocator_);
  if (result != VK_SUCCESS) {
    return util::unexpected(vk_error(result, "vmaCreateAllocator"));
  }

  QUARK_OK();
}

void Allocator::destroy() noexcept {
  if (allocator_ == nullptr) {
    return;
  }

  vmaDestroyAllocator(allocator_);
  allocator_ = nullptr;
}

} // namespace quark::vk