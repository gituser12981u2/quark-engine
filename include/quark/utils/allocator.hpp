#pragma once

#include <memory_resource>

namespace util {

using BumpAllocator = std::pmr::monotonic_buffer_resource;
using MemoryResource = std::pmr::memory_resource;

[[nodiscard]] inline MemoryResource *default_memory_resource() noexcept {
  return std::pmr::get_default_resource();
}

[[nodiscard]] inline MemoryResource *system_memory_resource() noexcept {
  return std::pmr::new_delete_resource();
}

} // namespace util