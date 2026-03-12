#pragma once

#include <quark/platform/window/IWindow.hpp>

namespace quark::platform {

/**
 * @brief Typed helper to query a capability interface from a window.
 *
 * @tparam T Capability interface type.
 *
 * @return Pointer to T if supported, otherwise nullptr.
 */
template <class T> T *query(IWindow &window) noexcept {
  return static_cast<T *>(window.query_interface(interface_id<T>()));
}

/**
 * @brief Const typed helper to query a capability interface from a window.
 */
template <class T> const T *query(const IWindow &window) noexcept {
  return static_cast<const T *>(window.query_interface(interface_id<T>()));
}

} // namespace quark::platform
