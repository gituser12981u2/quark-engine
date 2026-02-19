#pragma once

#include "quark/utils/diagnostic.hpp"
#include <cstddef>

namespace spdlog {
class logger;
}

namespace util {

struct SpdlogFileConfig {
  std::string_view file_path = "quark.log";
  std::size_t max_bytes = 50ULL * 1024ULL * 1024ULL;
  std::size_t max_files = 5;
};

struct SpdlogConsoleConfig {
  bool enable = true;
  bool to_stderr = true;
};

struct SpdlogPatternConfig {
  std::string_view file_pattern =
      "%Y-%m-%d %H:%M:%S.%e | %l | t:%t | %s:%# | %v";
  std::string_view console_pattern = "%^%l%$ | %s:%# | %v";
};

struct SpdlogLoggerConfig {
  std::string_view logger_name = "quark";
  SpdlogFileConfig file{};
  SpdlogConsoleConfig console{};
  SpdlogPatternConfig pattern{};
  int level = 0;
  int flush_on = 3;
};

/**
 * @brief Creates and configures the spd logger.
 *
 * @return The spd logger for ownership/lifetime control.
 */
[[nodiscard]] std::shared_ptr<spdlog::logger>
make_spdlog_logger(const SpdlogLoggerConfig &cfg);

/**
 * @brief A sink function for {@code DiagnosticSink{fn, ctx}} where ctx is
 * spdlog::logger*.
 */
void spdlog_sink(void *ctx, const util::DiagnosticEvent &e) noexcept;

} // namespace util
