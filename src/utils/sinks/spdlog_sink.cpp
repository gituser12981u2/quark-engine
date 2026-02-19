#include <quark/utils/diagnostic.hpp>
#include <quark/utils/sinks/spdlog_sink.hpp>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace util {

static spdlog::level::level_enum to_spd(util::Severity s) noexcept {
  switch (s) {
  case Severity::Info:
    return spdlog::level::info;
  case Severity::Warning:
    return spdlog::level::warn;
  case util::Severity::Error:
    return spdlog::level::err;
  }

  return spdlog::level::err;
}

static spdlog::level::level_enum level_from_int(int v) noexcept {
  switch (v) {
  case 0:
    return spdlog::level::trace;
  case 1:
    return spdlog::level::debug;
  case 2:
    return spdlog::level::info;
  case 3:
    return spdlog::level::warn;
  case 4:
    return spdlog::level::err;
  case 5:
    return spdlog::level::critical;
  case 6:
    return spdlog::level::off;
  default:
    return spdlog::level::trace;
  }
}

std::shared_ptr<spdlog::logger>
make_spdlog_logger(const SpdlogLoggerConfig &cfg) {
  std::vector<spdlog::sink_ptr> sinks;
  sinks.reserve(cfg.console.enable ? 2 : 1);

  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      std::string(cfg.file.file_path), cfg.file.max_bytes, cfg.file.max_files);
  file_sink->set_pattern(std::string(cfg.pattern.file_pattern));
  sinks.push_back(file_sink);

  if (cfg.console.enable) {
    spdlog::sink_ptr console_sink =
        cfg.console.to_stderr
            ? spdlog::sink_ptr{std::make_shared<
                  spdlog::sinks::stderr_color_sink_mt>()}
            : spdlog::sink_ptr{
                  std::make_shared<spdlog::sinks::stdout_color_sink_mt>()};

    console_sink->set_pattern(std::string(cfg.pattern.console_pattern));
    sinks.push_back(std::move(console_sink));
  }

  auto lg = std::make_shared<spdlog::logger>(std::string(cfg.logger_name),
                                             sinks.begin(), sinks.end());

  lg->set_level(level_from_int(cfg.level));
  lg->flush_on(level_from_int(cfg.flush_on));

  spdlog::register_logger(lg);
  return lg;
}

void spdlog_sink(void *ctx, const util::DiagnosticEvent &e) noexcept {
  auto *lg = static_cast<spdlog::logger *>(ctx);
  if (lg == nullptr) {
    return;
  }

  const auto &w = e.where;
  spdlog::source_loc loc{w.file_name(), static_cast<int>(w.line()),
                         w.function_name()};

  // Prefix module into message
  if (!e.module.empty()) {
    lg->log(loc, to_spd(e.severity), "[{}] {}", e.module, e.msg);
  } else {
    lg->log(loc, to_spd(e.severity), "{}", e.msg);
  }
}

} // namespace util
