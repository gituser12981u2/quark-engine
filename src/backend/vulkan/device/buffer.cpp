#include <cstddef>
#include <cstring>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/buffer.hpp>
#include <quark/vk/vk_error.hpp>

namespace quark::vk {

util::Status Buffer::create(const Buffer::CreateInfo &ci) {
  destroy();

  QUARK_ENSURE(ci.allocator != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "Buffer allocator is null"));
  QUARK_ENSURE(ci.size > 0,
               QUARK_ERR(util::Errc::InvalidArg, "Buffer size must be > 0"));
  QUARK_ENSURE(ci.usage != 0,
               QUARK_ERR(util::Errc::InvalidArg, "Buffer usage must be set"));

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = ci.size;
  buffer_info.usage = ci.usage;
  buffer_info.sharingMode = ci.sharing_mode;

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = ci.memory_usage;
  alloc_info.flags = ci.allocation_flags;

  VkResult result = vmaCreateBuffer(ci.allocator, &buffer_info, &alloc_info,
                                    &buffer_, &allocation_, nullptr);
  if (result != VK_SUCCESS) {
    return util::unexpected(vk_error(result, "vmaCreateBuffer"));
  }

  allocator_ = ci.allocator;
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
  QUARK_ENSURE(
      offset + static_cast<VkDeviceSize>(byte_count) <= size_,
      QUARK_ERR(util::Errc::InvalidArg, "Upload range exceeds buffer size"));

  void *mapped = nullptr;
  VkResult result = vmaMapMemory(allocator_, allocation_, &mapped);
  if (result != VK_SUCCESS) {
    return util::unexpected(vk_error(result, "vmaMapMemory"));
  }

  std::memcpy(static_cast<std::byte *>(mapped) + offset, src, byte_count);
  vmaUnmapMemory(allocator_, allocation_);
  QUARK_OK();
}

} // namespace quark::vk