#pragma once

#include <cstddef>
#include <memory>
#include <quark/utils/diagnostic.hpp>

namespace spdlog {
class logger;
}

namespace util {

/**
 * @brief Configuration for the rotating file sink/logger.
 *
 * Controls where the log file is written and how rotation is performed.
 */
struct SpdlogFileConfig {
  std::string_view file_path = "quark.log";
  std::size_t max_bytes = 50ULL * 1024ULL * 1024ULL;
  std::size_t max_files = 5;
};

/**
 * @brief Configuration for console sinks/loggers.
 *
 * The console output can be disabled and can be routed to stdout/stderr.
 */
struct SpdlogConsoleConfig {
  bool enable = true;
  bool to_stderr = true;
};

/**
 * @brief Pattern strings used by spdlog formatters.
 *
 * Patterns are applied per-logger, allowing different patterns for console vs
 * file.
 */
struct SpdlogPatternConfig {
  std::string_view file_pattern =
      "%Y-%m-%d %H:%M:%S.%e | %l | t:%t | %s:%# | %v";
  std::string_view console_info_pattern = "%^%l%$ | %v";
  std::string_view console_warn_pattern = "%^%l%$ | %s:%# | %v";
};

/**
 * @brief High level configuration for constructing the spdlog loggers.
 *
 * This bundles the file/console settings, patterns, and severity thresholds.
 */
struct SpdlogLoggerConfig {
  std::string_view logger_name = "quark";
  SpdlogFileConfig file{};
  SpdlogConsoleConfig console{};
  SpdlogPatternConfig pattern{};
  int level = 0;
  int flush_on = 3;
};

/**
 * @brief Runtime handles to the loggers/sinks created from {@code
 * SpdlogLoggerConfig}.
 *
 * There exists optional:
 * - rotating file logger for all messages
 * - a compact console logger for info/debug
 * - a verbose console logger for warnings/errors (without source location)
 */
struct SpdlogContext {
  std::shared_ptr<spdlog::logger> file;
  std::shared_ptr<spdlog::logger> console_info;
  std::shared_ptr<spdlog::logger> console_warn;
  bool console_enabled = false;
};

/**
 * @brief Creates and configures the spd logger.
 *
 * @return The spd logger for ownership/lifetime control.
 */
[[nodiscard]] std::shared_ptr<SpdlogContext>
make_spdlog_logger(const SpdlogLoggerConfig &cfg);

/**
 * @brief A sink function for {@code DiagnosticSink{fn, ctx}} where ctx is
 * spdlog::logger*.
 */
void spdlog_sink(void *ctx_ptr, const util::DiagnosticEvent &e) noexcept;

} // namespace util
