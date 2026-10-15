#include <array>
#include <limits>

#include <quark/utils/diagnostic.hpp>
#include <quark/vk/mesh.hpp>

namespace quark::vk {

util::Status Mesh::create(const Allocator &allocator,
                          std::span<const MeshVertex> vertices,
                          std::span<const uint32_t> indices) {
  destroy();

  QUARK_ENSURE(!vertices.empty(),
               QUARK_ERR(util::Errc::InvalidArg, "mesh has no vertices"));
  QUARK_ENSURE(!indices.empty(),
               QUARK_ERR(util::Errc::InvalidArg, "mesh has no indices"));
  QUARK_ENSURE(indices.size() <= std::numeric_limits<uint32_t>::max(),
               QUARK_ERR(util::Errc::InvalidArg, "mesh has too many indices"));

  QUARK_TRY_STATUS(vertex_buffer_.create({
      .allocator = &allocator,
      .size = vertices.size_bytes(),
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
      .allocation_flags =
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
  }));
  QUARK_TRY_STATUS(
      vertex_buffer_.upload(vertices.data(), vertices.size_bytes()));

  QUARK_TRY_STATUS(index_buffer_.create({
      .allocator = &allocator,
      .size = indices.size_bytes(),
      .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      .memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
      .allocation_flags =
          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
  }));
  QUARK_TRY_STATUS(index_buffer_.upload(indices.data(), indices.size_bytes()));

  index_count_ = static_cast<uint32_t>(indices.size());
  QUARK_OK();
}

void Mesh::destroy() noexcept {
  index_buffer_.destroy();
  vertex_buffer_.destroy();
  index_count_ = 0;
}

void Mesh::draw(VkCommandBuffer command_buffer) const noexcept {
  const VkDeviceSize offset = 0;
  const VkBuffer vertex_buffer = vertex_buffer_.handle();
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, &offset);
  vkCmdBindIndexBuffer(command_buffer, index_buffer_.handle(), 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(command_buffer, index_count_, 1, 0, 0, 0);
}

} // namespace quark::vk
