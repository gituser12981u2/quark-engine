#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <memory>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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
 * @brief Severity level for diagnostic events and errors.
 */
enum class Severity : std::uint8_t { Info, Warning, Error };

/**
 * @brief Engine error codes.
 */
enum class Errc : std::uint8_t {
  Unknown = 0,

  InvalidArg,
  InvalidState,
  NotFound,
  AlreadyExists,
  Unsupported,
  OutOfMemory,
  ResourceExhausted,
  Timeout,
  Cancelled,
  Internal,

  ApiError, ///< Generic external API failure
};

/**
 * @brief A diagnostic event is a structured log/trace message.
 *
 * This is used for info/warn/error/fatal reporting.
 */
struct DiagnosticEvent {
  Severity severity = Severity::Info;

  /// optional; sink derives from {@code where.file_name()}
  std::string_view module;

  std::string msg;
  std::source_location where = std::source_location::current();
};

/**
 * @brief An Error is a failure value suitable for returning via
 * {@code util::Result<T>}.
 *
 * It is also a {@code DiagnosticEvent} (severity defaults to {@code Error})
 * and:
 *   - an engine-level error code ({@code Errc})
 *   - an optional native/backend payload ({@code domain})
 *
 * @note Use {@code domain} on {@code Error} for backend/native error payloads
 * (e.g. VkResult, HRESULT, errno, etc.), and the formatted message for detail.
 */
struct Error {
  Errc code = Errc::Unknown;
  Severity severity = Severity::Error;

  /// Native/API payload (e.g. VkResult, HRESULT, errno, etc.)
  std::int32_t domain = 0;

  /// optional; sink derives from {@code where.file_name()}
  std::string_view module;

  std::string msg;
  std::source_location where = std::source_location::current();

  [[nodiscard]] constexpr operator DiagnosticEvent() const {
    DiagnosticEvent e;
    e.severity = severity;
    e.module = module;
    e.msg = msg;
    e.where = where;
    return e;
  }
};

/**
 * @brief Result type.
 */
template <class T> using Result = util::expected<T, Error>;

/**
 * @brief Success-or-Error status type.
 */
using Status = Result<void>;

using DiagnosticSinkFn = void (*)(void *ctx, const DiagnosticEvent &) noexcept;

struct DiagnosticSink {
  DiagnosticSinkFn fn = nullptr;
  void *ctx = nullptr;
};

using SinkList = std::vector<DiagnosticSink>;

void set_diagnostic_sinks(std::span<const DiagnosticSink> sinks) noexcept;
std::shared_ptr<const SinkList> diagnostic_sinks_snapshot() noexcept;

void report(const DiagnosticEvent &) noexcept;

template <class T>
inline void report_if_error(const util::Result<T> &r) noexcept {
  if (!r) {
    util::report(r.error());
  }
}

} // namespace util

#include <quark/utils/details/diagnostic_details.hpp>

#define QUARK_TRY_STATUS(expr)                                                 \
  do {                                                                         \
    auto _q_res = (expr);                                                      \
    if (!_q_res) {                                                             \
      return ::util::unexpected(std::move(_q_res.error()));                    \
    }                                                                          \
  } while (0)

#define QUARK_TRY_ASSIGN(lhs, expr)                                            \
  do {                                                                         \
    auto _q_res = (expr);                                                      \
    if (!_q_res) {                                                             \
      return ::util::unexpected(std::move(_q_res.error()));                    \
    }                                                                          \
    (lhs) = std::move(_q_res.value());                                         \
  } while (0)

#define QUARK_FAIL(err_expr)                                                   \
  do {                                                                         \
    return ::util::unexpected((err_expr));                                     \
  } while (0)

#define QUARK_ENSURE(cond, err_expr)                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      return ::util::unexpected((err_expr));                                   \
    }                                                                          \
  } while (0)

#define QUARK_LOG_INFO(fmtstr, ...)                                            \
  do {                                                                         \
    ::util::report(::util::details::make_event(                                \
        ::util::Severity::Info, {}, std::source_location::current(),           \
        (fmtstr)__VA_OPT__(, ) __VA_ARGS__));                                  \
  } while (0)

#define QUARK_LOG_WARN(fmtstr, ...)                                            \
  do {                                                                         \
    ::util::report(::util::details::make_event(                                \
        ::util::Severity::Warning, {}, std::source_location::current(),        \
        (fmtstr)__VA_OPT__(, ) __VA_ARGS__));                                  \
  } while (0)

#define QUARK_LOG_INFO_MOD(mod, fmtstr, ...)                                   \
  do {                                                                         \
    ::util::report(::util::details::make_event(                                \
        ::util::Severity::Info, (mod), std::source_location::current(),        \
        (fmtstr)__VA_OPT__(, ) __VA_ARGS__));                                  \
  } while (0)

#define QUARK_LOG_WARN_MOD(mod, fmtstr, ...)                                   \
  do {                                                                         \
    ::util::report(::util::details::make_event(                                \
        ::util::Severity::Warning, (mod), std::source_location::current(),     \
        (fmtstr)__VA_OPT__(, ) __VA_ARGS__));                                  \
  } while (0)

#define QUARK_ERR(code, fmtstr, ...)                                           \
  (::util::details::make_error((code), ::util::Severity::Error, {},            \
                               std::source_location::current(),                \
                               (fmtstr)__VA_OPT__(, ) __VA_ARGS__))

#define QUARK_ERR_MOD(code, mod, fmtstr, ...)                                  \
  (::util::details::make_error((code), ::util::Severity::Error, (mod),         \
                               std::source_location::current(),                \
                               (fmtstr)__VA_OPT__(, ) __VA_ARGS__))

#define QUARK_REPORT_IF_ERROR(res_expr)                                        \
  do {                                                                         \
    auto _q_res = (res_expr);                                                  \
    if (!_q_res) {                                                             \
      ::util::report(_q_res.error());                                          \
    }                                                                          \
  } while (0)
