#include "quark/rhi/shader/details/shader_registry.hpp"
#include "quark/rhi/shader/details/shader_handle.hpp"
#include "quark/rhi/shader/shader_file.hpp"

#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <span>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::rhi::details {

util::Status ShaderRegistry::create(const CreateInfo &ci) {
  QUARK_ENSURE(
      ci.device != VK_NULL_HANDLE,
      QUARK_ERR(util::Errc::InvalidArg, "shader registry device is null"));

  destroy();
  device_ = ci.device;
  QUARK_OK();
}

void ShaderRegistry::destroy() noexcept {
  slots_.clear();
  free_.clear();
  device_ = VK_NULL_HANDLE;
}

bool ShaderRegistry::alive(ShaderHandle handle) const noexcept {
  if (!handle.valid() || handle.index >= slots_.size()) {
    return false;
  }

  const Slot &slot = slots_[handle.index];
  return slot.live && slot.generation == handle.generation;
}

std::span<const uint32_t>
ShaderRegistry::code(ShaderHandle handle) const noexcept {
  if (!alive(handle)) {
    return {};
  }

  return slots_[handle.index].code;
}

util::Result<ShaderHandle>
ShaderRegistry::load_spv_file(const std::filesystem::path &path) {
  std::vector<uint32_t> code;
  QUARK_TRY_ASSIGN(code, read_spv_file(path));

  ShaderHandle handle = allocate_handle_();
  Slot &slot = slots_[handle.index];

  slot.code = std::move(code);
  slot.path = path;
  slot.live = true;

  return handle;
}

ShaderHandle ShaderRegistry::allocate_handle_() {
  uint32_t index{};

  if (!free_.empty()) {
    index = free_.back();
    free_.pop_back();
  } else {
    index = static_cast<uint32_t>(slots_.size());
    slots_.push_back(Slot{});
  }

  Slot &slot = slots_[index];
  return ShaderHandle{
      .index = index,
      .generation = slot.generation,
  };
}

} // namespace quark::rhi::details
