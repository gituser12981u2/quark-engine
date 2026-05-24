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
