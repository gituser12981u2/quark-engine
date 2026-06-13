#pragma once

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class Allocator final {
public:
  struct CreateInfo {
    VkInstance instance{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    const VkAllocationCallbacks *allocation_callbacks{nullptr};
    uint32_t vulkan_api_version{VK_API_VERSION_1_0};
  };

  Allocator() = default;
  ~Allocator() { destroy(); }

  QUARK_NO_COPY(Allocator);

  [[nodiscard]] constexpr Allocator(Allocator &&other) noexcept
      : allocator_(other.allocator_) {
    other.allocator_ = nullptr;
  }

  [[nodiscard]] constexpr Allocator &operator=(Allocator &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    destroy();
    allocator_ = other.allocator_;
    other.allocator_ = nullptr;
    return *this;
  }

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return allocator_ != nullptr; }
  [[nodiscard]] VmaAllocator handle() const noexcept { return allocator_; }

private:
  VmaAllocator allocator_ = nullptr;
};

} // namespace quark::vk