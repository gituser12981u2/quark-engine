#pragma once

#include <fmt/core.h>
#include <memory>
#include <quark/utils/error_types.hpp>
#include <quark/utils/result.hpp>
#include <span>
#include <vector>

namespace util {

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

/**
 * @def QUARK_TRY_STATUS(expr)
 * @brief Propagates a failing util::Status (util::Result<void>) to the caller.
 *
 * Evaluates @p expr exactly once and expects it to return util::Status
 * (i.e. util::Result<void>) or another util::Result like type with:
 * - boolean conversion indicating success
 * - .error() yielding util::Error
 *
 * On failure, returns util::unexpected(error) from the *current function*,
 * transferring ownership of the error via move.
 *
 * Typical use:
 * @code
 * util::Status f() {
 *   QUARK_TRY_STATUS(g());
 *   QUARK_OK();
 * }
 * @endcode
 *
 * Requirements:
 * 0 Must be used inside a function that returns util::Status (or compatible
 * util::Result<T> with matching error type).
 *
 * Notes:
 * - This macro does not modify the error's source_location; the error's
 *   where-field remains whatever the callee produced.
 */
#define QUARK_TRY_STATUS(expr)                                                 \
  do {                                                                         \
    auto _q_res = (expr);                                                      \
    if (!_q_res) {                                                             \
      return ::util::unexpected(std::move(_q_res.error()));                    \
    }                                                                          \
  } while (0)

/**
 * @def QUARK_TRY_ASSIGN(lhs, expr)
 * @brief Propagates failure; otherwise assigns the success value to @p lhs.
 *
 * Evaluates @p expr exactly once. @p expr must return util::Result<T> (or
 * compatible) with:
 * - boolean conversion indicating success
 * - .value() returning T
 * - .error() returning util::Error
 *
 * If @p expr fails, returns util::unexpected(error) from the current function.
 * If it succeeds, moves the value into @p lhs.
 *
 * Example:
 * @code
 * util::Result<int> make();
 * util::Status f() {
 *   int x = 0;
 *   QUARK_TRY_ASSIGN(x, make());
 *   // x initialized on success
 *   QUARK_OK();
 * }
 * @endcode
 *
 * Requirements:
 * - @p lhs must be assignable from the result value type.
 * - Current function must return a compatible util::Result<...>.
 */
#define QUARK_TRY_ASSIGN(lhs, expr)                                            \
  do {                                                                         \
    auto _q_res = (expr);                                                      \
    if (!_q_res) {                                                             \
      return ::util::unexpected(std::move(_q_res.error()));                    \
    }                                                                          \
    (lhs) = std::move(_q_res.value());                                         \
  } while (0)

/**
 * @def QUARK_TRY_VALIDATE(expr)
 * @brief Propagates failure like QUARK_TRY_STATUS, but rewrites source
 * location.
 *
 * This is intended specifically for validation style helper functions where
 * the error is *structurally correct* but the most actionable source location
 * is the call-site of validation, not the internal validation check.
 *
 * Behavior:
 * - Evaluates @p expr exactly once.
 * - On success: no-op.
 * - On failure:
 *   - takes ownership of the error
 *   - appends a note containing the original failure location
 *   - overwrites error.where with std::source_location::current()
 *   - returns util::unexpected(modified_error) from the current function
 *
 * This yields logs like:
 * - where = call-site of validate(...)
 * - msg includes "(validate failed at <file>:<line>)" showing the original
 * check
 *
 * Requirements:
 * - @p expr must return util::Status or util::Result-like.
 * - Current function must return util::Status (or compatible
 * util::Result<...>).
 *
 * Notes:
 * - This macro formats the augmented message using fmt::format; it therefore
 *   requires <fmt/format.h> (directly or indirectly) to be included.
 * - Use this macro only for validation entrypoints; do not use it for general
 *   error propagation or one will lose precise origin information.
 */
#define QUARK_TRY_VALIDATE(expr)                                               \
  do {                                                                         \
    auto _q_res = (expr);                                                      \
    if (!_q_res) {                                                             \
      auto _q_err = std::move(_q_res.error());                                 \
      _q_err.msg = fmt::format("{} (validate failed at {}:{})", _q_err.msg,    \
                               _q_err.where.file_name(), _q_err.where.line()); \
      _q_err.where = std::source_location::current();                          \
      return ::util::unexpected(std::move(_q_err));                            \
    }                                                                          \
  } while (0)

/**
 * @def QUARK_FAIL(err_expr)
 * @brief Returns util::unexpected(err_expr) from the current function.
 *
 * @p err_expr must produce a util::Error.
 *
 * Example:
 * @code
 * QUARK_FAIL(QUARK_ERR(util::Errc::InvalidArg, "x must be positive"));
 * @endcode
 *
 * Requirements:
 * - Current function must return util::Status or compatible util::Result<T>.
 */
#define QUARK_FAIL(err_expr)                                                   \
  do {                                                                         \
    return ::util::unexpected((err_expr));                                     \
  } while (0)

/**
 * @def QUARK_ENSURE(cond, err_expr)
 * @brief Enforces a condition; returns an error if the condition fails.
 *
 * If @p cond is false, returns util::unexpected(err_expr) from the current
 * function. Otherwise continues.
 *
 * @p err_expr must produce a util::Error.
 *
 * Example:
 * @code
 * QUARK_ENSURE(ptr != nullptr,
 *              QUARK_ERR(util::Errc::InvalidArg, "ptr is null"));
 * @endcode
 *
 * Requirements:
 * - Current function must return util::Status or compatible util::Result<T>.
 */
#define QUARK_ENSURE(cond, err_expr)                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      return ::util::unexpected((err_expr));                                   \
    }                                                                          \
  } while (0)

/**
 * @def QUARK_OK()
 * @brief Convenience return for successful util::Status.
 *
 * Equivalent to:
 * @code
 * return util::Status{};
 * @endcode
 *
 * Use to avoid repetitive `return {};` at the end of util::Status functions.
 *
 * I like rust.
 */
#define QUARK_OK()                                                             \
  return ::util::Status {}

/**
 * @def QUARK_LOG_INFO(fmtstr, ...)
 * @brief Emits an informational diagnostic event.
 *
 * Formats @p fmtstr with fmt (via make_event) and reports it to the configured
 * diagnostic sinks. The event source location is
 * std::source_location::current() at the call site.
 *
 * @param fmtstr A fmt-compatible format string.
 * @param ...    Optional fmt arguments.
 */
#define QUARK_LOG_INFO(fmtstr, ...)                                            \
  do {                                                                         \
    ::util::report(::util::details::make_event(                                \
        ::util::Severity::Info, {}, std::source_location::current(),           \
        (fmtstr)__VA_OPT__(, ) __VA_ARGS__));                                  \
  } while (0)

/**                                                                            \
 * @def QUARK_LOG_WARN(fmtstr, ...)                                            \
 * @brief Emits a warning diagnostic event.                                    \
 *                                                                             \
 * Formats @p fmtstr with fmt (via make_event) and reports it to the           \
 * configured diagnostic sinks. The event source location is                   \
 * std::source_location::current() at the call site.                           \
 *                                                                             \
 * @param fmtstr A fmt-compatible format string.                               \
 * @param ...    Optional fmt arguments.                                       \
 */                                                                            \
#define QUARK_LOG_WARN(fmtstr, ...)                                            \
  do {                                                                         \
    ::util::report(::util::details::make_event(                                \
        ::util::Severity::Warning, {}, std::source_location::current(),        \
        (fmtstr)__VA_OPT__(, ) __VA_ARGS__));                                  \
  } while (0)

/**
 * @def QUARK_LOG_INFO_MOD(mod, fmtstr, ...)
 * @brief Emits an informational diagnostic event with an explicit module tag.
 *
 * @param mod    Module/category name (e.g. "vk.instance", "io.fs").
 * @param fmtstr A fmt-compatible format string.
 * @param ...    Optional fmt arguments.
 */
#define QUARK_LOG_INFO_MOD(mod, fmtstr, ...)                                   \
  do {                                                                         \
    ::util::report(::util::details::make_event(                                \
        ::util::Severity::Info, (mod), std::source_location::current(),        \
        (fmtstr)__VA_OPT__(, ) __VA_ARGS__));                                  \
  } while (0)

/**
 * @def QUARK_LOG_WARN_MOD(mod, fmtstr, ...)
 * @brief Emits a warning diagnostic event with an explicit module tag.
 *
 * @param mod    Module/category name (e.g. "vk.instance", "io.fs").
 * @param fmtstr A fmt-compatible format string.
 * @param ...    Optional fmt arguments.
 */
#define QUARK_LOG_WARN_MOD(mod, fmtstr, ...)                                   \
  do {                                                                         \
    ::util::report(::util::details::make_event(                                \
        ::util::Severity::Warning, (mod), std::source_location::current(),     \
        (fmtstr)__VA_OPT__(, ) __VA_ARGS__));                                  \
  } while (0)

/**
 * @def QUARK_ERR(code, fmtstr, ...)
 * @brief Constructs a util::Error with severity Error and current source
 * location.
 *
 * This does not report/log the error; it merely constructs it so it can be
 * returned via util::unexpected(...) or otherwise handled.
 *
 * @param code   Engine level error code (util::Errc).
 * @param fmtstr A fmt compatible format string for the error message.
 * @param ...    Optional fmt arguments.
 *
 * @return util::Error with:
 * - .code = @p code
 * - .severity = util::Severity::Error
 * - .module = empty (sink may infer from file)
 * - .where = call-site source location
 */
#define QUARK_ERR(code, fmtstr, ...)                                           \
  (::util::details::make_error((code), ::util::Severity::Error, {},            \
                               std::source_location::current(),                \
                               (fmtstr)__VA_OPT__(, ) __VA_ARGS__))

/**
 * @def QUARK_ERR_MOD(code, mod, fmtstr, ...)
 * @brief Constructs a util::Error with an explicit module tag.
 *
 * This does not report/log the error; it merely constructs it so it can be
 * returned via util::unexpected(...) or otherwise handled.
 *
 * @param code   Engine level error code (util::Errc).
 * @param fmtstr A fmt compatible format string for the error message.
 * @param ...    Optional fmt arguments.
 *
 * @return util::Error with:
 * - .code = @p code
 * - .severity = util::Severity::Error
 * - .module = empty (sink may infer from file)
 * - .where = call-site source location
 */
#define QUARK_ERR_MOD(code, mod, fmtstr, ...)                                  \
  (::util::details::make_error((code), ::util::Severity::Error, (mod),         \
                               std::source_location::current(),                \
                               (fmtstr)__VA_OPT__(, ) __VA_ARGS__))

/**
 * @def QUARK_REPORT_IF_ERROR(res_expr)
 * @brief Evaluates a util::Result expression and reports the error if it
 * failed.
 *
 * Evaluates @p res_expr exactly once. If it yields an error, the error is
 * reported to diagnostic sinks. The result is not returned or propagated.
 *
 * Intended for best-effort cleanup paths where failures are non-fatal.
 *
 * Example:
 * @code
 * QUARK_REPORT_IF_ERROR(try_close_file());
 * @endcode
 */
#define QUARK_REPORT_IF_ERROR(res_expr)                                        \
  do {                                                                         \
    auto _q_res = (res_expr);                                                  \
    if (!_q_res) {                                                             \
      ::util::report(_q_res.error());                                          \
    }                                                                          \
  } while (0)
