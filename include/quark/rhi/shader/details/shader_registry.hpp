#pragma once

#include "quark/rhi/shader/details/shader_handle.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace quark::rhi::details {

class ShaderRegistry final {
public:
  struct CreateInfo {
    VkDevice device{VK_NULL_HANDLE};
  };

  ShaderRegistry() = default;
  ~ShaderRegistry() { destroy(); }

  QUARK_MOVE_ONLY(ShaderRegistry);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] util::Result<ShaderHandle>
  load_spv_file(const std::filesystem::path &path);

  [[nodiscard]] bool alive(ShaderHandle handle) const noexcept;

  [[nodiscard]] std::span<const uint32_t>
  code(ShaderHandle handle) const noexcept;

private:
  struct Slot {
    bool live{false};
    uint32_t generation{1};

    std::vector<uint32_t> code;
    std::filesystem::path path;
  };

  [[nodiscard]] ShaderHandle allocate_handle_();

  VkDevice device_{VK_NULL_HANDLE};
  std::vector<Slot> slots_;
  std::vector<uint32_t> free_;
};

} // namespace quark::rhi::details
