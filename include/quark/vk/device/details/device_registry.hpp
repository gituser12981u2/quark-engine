#pragma once

#include "device.hpp"
#include "device_handle.hpp"

#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vector>

namespace quark::vk::details {

class DeviceRegistry final {
public:
  DeviceRegistry() = default;
  ~DeviceRegistry() = default;

  QUARK_MOVE_ONLY(DeviceRegistry);

  void clear() noexcept;

  [[nodiscard]] util::Result<DeviceHandle> create(const Device::CreateInfo &ci);
  void destroy(DeviceHandle handle) noexcept;

  [[nodiscard]] bool alive(DeviceHandle handle) const noexcept;

  Device *get(DeviceHandle handle) noexcept;
  [[nodiscard]] const Device *get(DeviceHandle handle) const noexcept;

private:
  struct Slot {
    Device device;
    uint32_t generation = 1;
    bool live = false;
  };

  [[nodiscard]] static bool matches_(DeviceHandle handle,
                                     const Slot &s) noexcept;

  std::vector<Slot> slots_;
  std::vector<uint32_t> free_;
};

} // namespace quark::vk::details
