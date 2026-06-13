#include <cstddef>
#include <limits>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/buffer.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/vk_error.hpp>

namespace quark::vk {

util::Status Buffer::create(const Buffer::CreateInfo &ci) {
  destroy();

  QUARK_ENSURE(ci.allocator != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "Buffer allocator is null"));
  QUARK_ENSURE(
      ci.allocator->valid(),
      QUARK_ERR(util::Errc::InvalidArg, "Buffer allocator is not created"));
  QUARK_ENSURE(ci.size > 0,
               QUARK_ERR(util::Errc::InvalidArg, "Buffer size must be > 0"));
  QUARK_ENSURE(ci.usage != 0,
               QUARK_ERR(util::Errc::InvalidArg, "Buffer usage must be set"));

  VmaAllocator allocator = ci.allocator->handle();

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = ci.size;
  buffer_info.usage = ci.usage;
  buffer_info.sharingMode = ci.sharing_mode;

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = ci.memory_usage;
  alloc_info.flags = ci.allocation_flags;

  QUARK_VK_TRY(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer_,
                               &allocation_, nullptr));

  allocator_ = allocator;
  size_ = ci.size;
  QUARK_OK();
}

void Buffer::destroy() noexcept {
  if (allocator_ != nullptr && buffer_ != VK_NULL_HANDLE &&
      allocation_ != nullptr) {
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
  }

  allocator_ = nullptr;
  buffer_ = VK_NULL_HANDLE;
  allocation_ = nullptr;
  size_ = 0;
}

util::Status Buffer::upload(const void *src, size_t byte_count,
                            VkDeviceSize offset) {
  QUARK_ENSURE(valid(),
               QUARK_ERR(util::Errc::InvalidState, "Buffer is not created"));
  QUARK_ENSURE(src != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "Upload source is null"));
  QUARK_ENSURE(offset <= size_, QUARK_ERR(util::Errc::InvalidArg,
                                          "Upload offset exceeds buffer size"));
  QUARK_ENSURE(byte_count <= (std::numeric_limits<VkDeviceSize>::max)(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "Upload byte_count exceeds VkDeviceSize"));

  const VkDeviceSize copy_size = static_cast<VkDeviceSize>(byte_count);
  QUARK_ENSURE(
      copy_size <= (size_ - offset),
      QUARK_ERR(util::Errc::InvalidArg, "Upload range exceeds buffer size"));

  QUARK_VK_TRY(vmaCopyMemoryToAllocation(allocator_, src, allocation_, offset,
                                         copy_size));
  QUARK_OK();
}

} // namespace quark::vk