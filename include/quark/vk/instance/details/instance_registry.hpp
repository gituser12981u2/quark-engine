#pragma once

#include "instance.hpp"
#include "instance_handle.hpp"

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

class InstanceRegistry final {
public:
  InstanceRegistry() = default;
  ~InstanceRegistry() = default;

  QUARK_MOVE_ONLY(InstanceRegistry);

  void clear() noexcept;

  [[nodiscard]] InstanceHandle create(const Instance::CreateInfo &ci);
  void destroy(InstanceHandle handle) noexcept;

  [[nodiscard]] bool alive(InstanceHandle handle) const noexcept;

  Instance *get(InstanceHandle handle) noexcept;
  [[nodiscard]] const Instance *get(InstanceHandle handle) const noexcept;

private:
  struct Slot {
    Instance instance;
    uint32_t generation = 1;
    bool live = false;
  };

  [[nodiscard]] bool matches_(InstanceHandle handle,
                              const Slot &s) const noexcept;

  std::vector<Slot> slots_;
  std::vector<uint32_t> free_;
};

} // namespace quark::vk
