#pragma once

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <quark/vk/instance/details/instance_handle.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace quark::vk::details {

class InstanceRegistry final {
public:
  InstanceRegistry() = default;
  ~InstanceRegistry() = default;

  QUARK_MOVE_ONLY(InstanceRegistry);

  void clear() noexcept;

  [[nodiscard]] util::Result<InstanceHandle>
  create(const Instance::CreateInfo &ci);
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

  [[nodiscard]] static bool matches_(InstanceHandle handle,
                                     const Slot &s) noexcept;

  std::vector<Slot> slots_;
  std::vector<uint32_t> free_;
};

} // namespace quark::vk::details
