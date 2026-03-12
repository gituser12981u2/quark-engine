#pragma once

#include <cstdint>
#include <source_location>
#include <string>
#include <string_view>

namespace util {

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

using DiagnosticSinkFn = void (*)(void *ctx, const DiagnosticEvent &) noexcept;

struct DiagnosticSink {
  DiagnosticSinkFn fn = nullptr;
  void *ctx = nullptr;
};

} // namespace util
