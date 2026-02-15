#pragma once

#include <array>
#include <cstddef>
#include <quark/platform/window/IWindow.hpp>
#include <quark/utils/raii.hpp>

namespace quark::platform {

/**
 * @brief Small fixed-capacity cache for capability interface instances.
 *
 * Windows often expose a small number of backend capabilities. This cache
 * stores created adapter objects and destroys them on clear() or destruction.
 *
 * @note The cache does not enforce ordering between cached object destruction
 * and external resources; callers should clear() before tearing down any
 * resources that cached adapters may reference.
 */
class InterfaceCache {
public:
  InterfaceCache() = default;
  ~InterfaceCache() { clear(); }

  QUARK_NO_COPY_NO_MOVE(InterfaceCache);

  void clear() noexcept {
    for (std::size_t i = 0; i < count_; ++i) {
      if (entries_[i].ptr != nullptr && entries_[i].deleter != nullptr) {
        entries_[i].deleter(entries_[i].ptr);
      }

      entries_[i] = Entry{};
    }

    count_ = 0;
  }

  [[nodiscard]] void *find(InterfaceId id) noexcept {
    for (std::size_t i = 0; i < count_; ++i) {
      if (entries_[i].id == id) {
        return entries_[i].ptr;
      }
    }

    return nullptr;
  }

  [[nodiscard]] const void *find(InterfaceId id) const noexcept {
    for (std::size_t i = 0; i < count_; ++i) {
      if (entries_[i].id == id) {
        return entries_[i].ptr;
      }
    }

    return nullptr;
  }

  /**
   * @brief Adds a cached interface pointer with a deleter.
   *
   * @param id Interface identifier
   * @param ptr Pointer to the interface object
   * @param deleter Function invoked as deleter(ptr) during clear()
   * @return true on success, false if capacity is exceeded
   */
  [[nodiscard]] bool add(InterfaceId id, void *ptr,
                         void (*deleter)(void *) noexcept) noexcept {
    if (count_ >= entries_.size()) {
      return false;
    }

    entries_[count_++] = Entry{.id = id, .ptr = ptr, .deleter = deleter};
    return true;
  }

private:
  struct Entry {
    InterfaceId id{};
    void *ptr = nullptr;
    void (*deleter)(void *) noexcept = nullptr;
  };

  static constexpr std::size_t kMax = 8;
  std::array<Entry, kMax> entries_{};
  std::size_t count_ = 0;
};

} // namespace quark::platform
