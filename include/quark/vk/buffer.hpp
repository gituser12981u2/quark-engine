#pragma once

#include <cstddef>

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class Buffer final {

public:
  struct CreateInfo {
    VmaAllocator allocator = nullptr;
    VkDeviceSize size{};
    VkBufferUsageFlags usage{};
    VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags allocation_flags{};
    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
  };

  Buffer() = default;
  ~Buffer() { destroy(); }

  QUARK_NO_COPY(Buffer); // do we want copy or not?

  Buffer(Buffer &&other) noexcept
      : allocator_(other.allocator_), buffer_(other.buffer_),
        allocation_(other.allocation_), size_(other.size_) {
    other.allocator_ = nullptr;
    other.buffer_ = VK_NULL_HANDLE;
    other.allocation_ = nullptr;
    other.size_ = 0;
  }

  Buffer &operator=(Buffer &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    this->destroy();
    allocator_ = other.allocator_;
    buffer_ = other.buffer_;
    allocation_ = other.allocation_;
    size_ = other.size_;

    other.allocator_ = nullptr;
    other.buffer_ = VK_NULL_HANDLE;
    other.allocation_ = nullptr;
    other.size_ = 0;
    return *this;
  }

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  util::Status upload(const void *src, size_t byte_count,
                      VkDeviceSize offset = 0);

  [[nodiscard]] bool valid() noexcept { return buffer_ != VK_NULL_HANDLE; }

  [[nodiscard]] VkBuffer handle() const noexcept { return buffer_; }
  [[nodiscard]] VkDeviceSize size() const noexcept { return size_; }

private:
  VmaAllocator allocator_{nullptr};
  VkBuffer buffer_{VK_NULL_HANDLE};
  VmaAllocation allocation_{nullptr};
  VkDeviceSize size_{};
};

} // namespace quark::vk