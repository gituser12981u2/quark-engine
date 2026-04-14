#pragma once

#include <cstdint>
#include <memory_resource>
#include <quark/utils/allocator.hpp>
#include <quark/utils/generic_handle.hpp>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vector>

namespace util {

template <class T, OpaqueHandle Handle> class GenerationalRegistry final {
public:
  using CreateInfo = typename T::CreateInfo;

  explicit GenerationalRegistry(
      std::pmr::memory_resource *memory_resource = default_memory_resource())
      : slots_(memory_resource), free_(memory_resource) {}

  ~GenerationalRegistry() = default;

  QUARK_MOVE_ONLY(GenerationalRegistry);

  void clear() noexcept {
    for (auto &slot : slots_) {
      if (slot.live) {
        slot.object.destroy();
        slot.live = false;
        ++slot.generation;
      }
    }

    free_.clear();
    free_.reserve(slots_.size());
    for (uint32_t index = 0; index < slots_.size(); ++index) {
      free_.push_back(index);
    }
  }

  [[nodiscard]] util::Result<Handle> create(const CreateInfo &create_info) {
    uint32_t index = 0;

    if (!free_.empty()) {
      index = free_.back();
      free_.pop_back();
    } else {
      index = static_cast<uint32_t>(slots_.size());
      slots_.emplace_back();
    }

    Slot &slot = slots_[index];

    if (slot.live) {
      slot.object.destroy();
      slot.live = false;
      ++slot.generation;
    }

    QUARK_TRY_STATUS(slot.object.create(create_info));
    slot.live = true;

    return Handle{.index = index, .generation = slot.generation};
  }

  void destroy(Handle handle) noexcept {
    if (!handle.valid() || handle.index >= slots_.size()) {
      return;
    }

    Slot &slot = slots_[handle.index];
    if (!matches_(handle, slot)) {
      return;
    }

    slot.object.destroy();
    slot.live = false;
    ++slot.generation;

    free_.push_back(handle.index);
  }

  [[nodiscard]] bool alive(Handle handle) const noexcept {
    if (!handle.valid() || handle.index >= slots_.size()) {
      return false;
    }

    return matches_(handle, slots_[handle.index]);
  }

  T *get(Handle handle) noexcept {
    if (!alive(handle)) {
      return nullptr;
    }

    return &slots_[handle.index].object;
  }

  [[nodiscard]] const T *get(Handle handle) const noexcept {
    if (!alive(handle)) {
      return nullptr;
    }

    return &slots_[handle.index].object;
  }

private:
  struct Slot {
    T object;
    uint32_t generation = 1;
    bool live = false;
  };

  [[nodiscard]] static bool matches_(Handle handle, const Slot &slot) noexcept {
    return handle.valid() && slot.live && slot.generation == handle.generation;
  }

  std::pmr::vector<Slot> slots_;
  std::pmr::vector<uint32_t> free_;
};

} // namespace util