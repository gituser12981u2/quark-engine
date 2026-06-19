#include <cstddef>
#include <limits>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/buffer.hpp>
#include <quark/vk/diagnostic_prelude.hpp>
#include <quark/vk/vk_error.hpp>

namespace quark::vk {

void Buffer::StateDeleter::operator()(State *state) const noexcept {
  if (state == nullptr) {
    return;
  }

  if (state->allocator != nullptr && state->buffer != VK_NULL_HANDLE &&
      state->allocation != nullptr) {
    vmaDestroyBuffer(state->allocator, state->buffer, state->allocation);
  }

  delete state;
}

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

  auto state = std::unique_ptr<State, StateDeleter>(new State{});
  state->allocator = allocator;

  QUARK_VK_TRY(vmaCreateBuffer(allocator, &buffer_info, &alloc_info,
                               &state->buffer, &state->allocation, nullptr));

  state->size = ci.size;
  state_ = std::move(state);

  QUARK_OK();
}

void Buffer::destroy() noexcept { state_.reset(); }

util::Status Buffer::upload(const void *src, size_t byte_count,
                            VkDeviceSize offset) {
  QUARK_ENSURE(valid(),
               QUARK_ERR(util::Errc::InvalidState, "Buffer is not created"));
  QUARK_ENSURE(src != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "Upload source is null"));
  QUARK_ENSURE(
      offset <= state_->size,
      QUARK_ERR(util::Errc::InvalidArg, "Upload offset exceeds buffer size"));
  QUARK_ENSURE(byte_count <= (std::numeric_limits<VkDeviceSize>::max)(),
               QUARK_ERR(util::Errc::InvalidArg,
                         "Upload byte_count exceeds VkDeviceSize"));

  const VkDeviceSize copy_size = static_cast<VkDeviceSize>(byte_count);
  QUARK_ENSURE(
      copy_size <= (state_->size - offset),
      QUARK_ERR(util::Errc::InvalidArg, "Upload range exceeds buffer size"));

  QUARK_VK_TRY(vmaCopyMemoryToAllocation(
      state_->allocator, src, state_->allocation, offset, copy_size));
  QUARK_OK();
}

} // namespace quark::vk