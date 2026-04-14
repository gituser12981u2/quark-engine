#include "quark/utils/error_types.hpp"
#include <memory>
#include <quark/utils/sinks/spdlog_sink.hpp>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>

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

std::shared_ptr<SpdlogContext>
make_spdlog_logger(const SpdlogLoggerConfig &cfg) {
  auto ctx = std::make_shared<SpdlogContext>();

  // File logger
  {
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        std::string(cfg.file.file_path), cfg.file.max_bytes,
        cfg.file.max_files);
    file_sink->set_pattern(std::string(cfg.pattern.file_pattern));

    ctx->file = std::make_shared<spdlog::logger>(std::string(cfg.logger_name),
                                                 file_sink);
    ctx->file->set_level(level_from_int(cfg.level));
    ctx->file->flush_on(level_from_int(cfg.flush_on));
    spdlog::register_logger(ctx->file);
  }

  // Console logger
  ctx->console_enabled = cfg.console.enable;
  if (cfg.console.enable) {
    auto make_console_sink = [&]() -> spdlog::sink_ptr {
      if (cfg.console.to_stderr) {
        return std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
      }
      return std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    };

    // Console with no location in pattern
    {
      auto s = make_console_sink();
      s->set_pattern(std::string(cfg.pattern.console_info_pattern)); // no %s:%#

      ctx->console_info = std::make_shared<spdlog::logger>(
          std::string(cfg.logger_name) + ".console_info", s);
      ctx->console_info->set_level(spdlog::level::info);
      spdlog::register_logger(ctx->console_info);
    }

    // Warn/Error console with included location
    {
      auto s = make_console_sink();
      // static constexpr const char *kWarnPattern = "%^%l%$ | %s:%# | %v";
      s->set_pattern(std::string(cfg.pattern.console_warn_pattern));

      ctx->console_warn = std::make_shared<spdlog::logger>(
          std::string(cfg.logger_name) + ".console_warn", s);
      ctx->console_warn->set_level(spdlog::level::warn);
      ctx->console_warn->flush_on(spdlog::level::warn);
      spdlog::register_logger(ctx->console_warn);
    }
  }

  return ctx;
}

void spdlog_sink(void *ctx_ptr, const util::DiagnosticEvent &e) noexcept {
  auto *ctx = static_cast<SpdlogContext *>(ctx_ptr);
  if (ctx == nullptr || !ctx->file) {
    return;
  }

  const auto &w = e.where;
  spdlog::source_loc const loc{w.file_name(), static_cast<int>(w.line()),
                               w.function_name()};

  if (!e.module.empty()) {
    ctx->file->log(loc, to_spd(e.severity), "[{}] {}", e.module, e.msg);
  } else {
    ctx->file->log(loc, to_spd(e.severity), "{}", e.msg);
  }

  if (!ctx->console_enabled) {
    return;
  }

  // Console routing: info -> no loc; warn/error -> with loc.
  if (e.severity == util::Severity::Info) {
    if (!ctx->console_info) {
      return;
    }

    if (!e.module.empty()) {
      ctx->console_info->log(to_spd(e.severity), "[{}] {}", e.module, e.msg);
    } else {
      ctx->console_info->log(to_spd(e.severity), "{}", e.msg);
    }
  } else {
    if (!ctx->console_warn) {
      return;
    }

    if (!e.module.empty()) {
      ctx->console_warn->log(loc, to_spd(e.severity), "[{}] {}", e.module,
                             e.msg);
    } else {
      ctx->console_warn->log(loc, to_spd(e.severity), "{}", e.msg);
    }
  }
}

} // namespace util
