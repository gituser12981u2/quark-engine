#pragma once

#include <quark/utils/error_types.hpp>

#if __has_include(<expected>) && defined(__cpp_lib_expected)
#include <expected>
#else
#include <tl/expected.hpp>
#endif

namespace util {

#if __has_include(<expected>) && defined(__cpp_lib_expected)

template <class T, class E> using expected = std::expected<T, E>;

template <class E> [[nodiscard]] inline auto unexpected(E &&e) {
  return std::unexpected<std::remove_cvref_t<E>>(std::forward<E>(e));
}
#else

template <class T, class E> using expected = tl::expected<T, E>;

template <class E> [[nodiscard]] inline auto unexpected(E &&e) {
  return tl::unexpected<std::remove_cvref_t<E>>(std::forward<E>(e));
}
#endif

/**
 * @brief Result type.
 */
template <class T> using Result = util::expected<T, Error>;

/**
 * @brief Success-or-Error status type.
 */
using Status = Result<void>;

} // namespace util
