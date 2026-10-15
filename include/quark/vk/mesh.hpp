#pragma once

#include <array>
#include <cstdint>
#include <span>

#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/allocator.hpp>
#include <quark/vk/buffer.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

struct MeshVertex {
  std::array<float, 3> position{};
  std::array<float, 3> color{};
};

class Mesh final {
public:
  Mesh() = default;
  ~Mesh() = default;

  QUARK_NO_COPY_NO_MOVE(Mesh);

  [[nodiscard]] util::Status create(const Allocator &allocator,
                                    std::span<const MeshVertex> vertices,
                                    std::span<const uint32_t> indices);
  void destroy() noexcept;
  void draw(VkCommandBuffer command_buffer) const noexcept;

private:
  Buffer vertex_buffer_;
  Buffer index_buffer_;
  uint32_t index_count_{};
};

} // namespace quark::vk
