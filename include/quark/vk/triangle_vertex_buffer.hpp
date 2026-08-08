#pragma once

#include <cstdint>

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/allocator.hpp>
#include <quark/vk/buffer.hpp>

namespace quark::vk {

class TriangleVertexBuffer final {
public:
  TriangleVertexBuffer() = default;
  ~TriangleVertexBuffer() { destroy(); }

  QUARK_NO_COPY_NO_MOVE(TriangleVertexBuffer);

  util::Status create(const Allocator &allocator);
  void destroy() noexcept;

  void bind(VkCommandBuffer command_buffer) const noexcept;
  [[nodiscard]] uint32_t vertex_count() const noexcept { return vertex_count_; }

private:
  Buffer vertex_buffer_;
  uint32_t vertex_count_{};
};

} // namespace quark::vk