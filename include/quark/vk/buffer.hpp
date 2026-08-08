#pragma once

#include <cstddef>
#include <memory>

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/allocator.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class Buffer final {

public:
  struct CreateInfo {
    const Allocator *allocator = nullptr;
    VkDeviceSize size{};
    VkBufferUsageFlags usage{};
    VmaMemoryUsage memory_usage{VMA_MEMORY_USAGE_AUTO};
    VmaAllocationCreateFlags allocation_flags{};
    VkSharingMode sharing_mode{VK_SHARING_MODE_EXCLUSIVE};
  };

  Buffer() = default;
  ~Buffer() { destroy(); }

  QUARK_MOVE_ONLY(Buffer);

  util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  util::Status upload(const void *src, size_t byte_count,
                      VkDeviceSize offset = 0);

  [[nodiscard]] bool valid() noexcept {
    return state_ != nullptr && state_->buffer != VK_NULL_HANDLE;
  }

  [[nodiscard]] VkBuffer handle() const noexcept {
    return state_ == nullptr ? VK_NULL_HANDLE : state_->buffer;
  }
  [[nodiscard]] VkDeviceSize size() const noexcept {
    return state_ == nullptr ? 0 : state_->size;
  }

private:
  struct State {
    VmaAllocator allocator{nullptr};
    VkBuffer buffer{VK_NULL_HANDLE};
    VmaAllocation allocation{nullptr};
    VkDeviceSize size{};
  };

  struct StateDeleter {
    void operator()(State *state) const noexcept;
  };

  std::unique_ptr<State, StateDeleter> state_;
};

} // namespace quark::vk