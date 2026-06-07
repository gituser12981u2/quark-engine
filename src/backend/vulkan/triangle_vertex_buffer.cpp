#include <array>

#include <quark/utils/diagnostic.hpp>
#include <quark/vk/triangle_vertex_buffer.hpp>

using std::array;

namespace quark::vk {

util::Status TriangleVertexBuffer::create(VmaAllocator allocator) {
  // Vertex data: 3 vertices, each with vec2 position and vec3 color.
  constexpr array<float, 15> triangle_vertices = {
      //  x,     y,     r,   g,   b
      0.0F,  -0.5F, 1.0F, 0.0F, 0.0F, // bottom center, red
      0.5F,  0.5F,  0.0F, 1.0F, 0.0F, // top right, green
      -0.5F, 0.5F,  0.0F, 0.0F, 1.0F  // top left, blue
  };

  constexpr VkDeviceSize buffer_size = sizeof(triangle_vertices);

  QUARK_TRY_STATUS(vertex_buffer_.create({
      .allocator = allocator,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
      .allocation_flags =
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
  }));

  QUARK_TRY_STATUS(vertex_buffer_.upload(triangle_vertices.data(),
                                         sizeof(triangle_vertices)));

  vertex_count_ = 3;
  QUARK_OK();
}

void TriangleVertexBuffer::destroy() noexcept {
  vertex_buffer_.destroy();
  vertex_count_ = 0;
}

void TriangleVertexBuffer::bind(VkCommandBuffer command_buffer) const noexcept {
  const array<VkDeviceSize, 1> offsets{0};
  auto *const vertex_buffer = vertex_buffer_.handle();
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets.data());
}

} // namespace quark::vk