#pragma once

#include <quark/utils/diagnostic.hpp>
#include <fmt/format.h>
#include <source_location>
#include <string_view>

namespace util::details {

/**
 * @brief Trim an absolute path to a stable project-relative path.
 *
 * Hueristic: if the path contains "/quark-engine/", returns substring after it.
 * Otherwise, returns the original.
 */
[[nodiscard]] constexpr std::string_view
make_relative(std::string_view abs) noexcept {
  constexpr std::string_view marker = "/quark-engine/";
  const auto pos = abs.find(marker);
  if (pos == std::string_view::npos) {
    return abs;
  }
  return abs.substr(pos + marker.size());
}

/**
 * @brief Derive a module tag from a relative path.
 *
 * Rule:
 * - "src/backend/<api>/..." -> "<api>"
 * - "src/<module>/..." -> "<module>"
 */
[[nodiscard]] constexpr std::string_view
derive_module_from_relative(std::string_view rel) noexcept {
  auto next_component = [](std::string_view s) constexpr -> std::string_view {
    const auto slash = s.find('/');
    if (slash == std::string_view::npos) {
      return s;
    }

    return s.substr(0, slash);
  };

  auto after_prefix =
      [](std::string_view s,
         std::string_view prefix) constexpr -> std::string_view {
    if (!s.starts_with(prefix)) {
      return {};
    }

    return s.substr(prefix.size());
  };

  if (auto rest = after_prefix(rel, "src/backend/"); !rest.empty()) {
    // src/backend/<api>/...
    const auto api = next_component(rest);
    return api.empty() ? std::string_view{"unknown"} : api;
  }

  if (auto rest = after_prefix(rel, "src/"); !rest.empty()) {
    // src/<module>/...
    const auto mod = next_component(rest);
    return mod.empty() ? std::string_view{"unknown"} : mod;
  }

  return "unknown";
}

[[nodiscard]] constexpr std::string_view
derive_module(std::source_location loc) noexcept {
  const std::string_view abs = loc.file_name();
  const std::string_view rel = make_relative(abs);
  return derive_module_from_relative(rel);
}

/**
 * @brief Make a DiagnosticEvent with formatting.
 *
 * @param severity Event severity.
 * @param module Short module tag (prefer string literal/static storage).
 * @param fmtstr fmt format string.
 * @param args fmt arguments.
 * @param loc Source location (defaults to call site).
 */
template <class... Args>
[[nodiscard]] inline DiagnosticEvent
make_event(Severity severity, std::string_view module, std::source_location loc,
           fmt::format_string<Args...> fmtstr, Args &&...args) {
  DiagnosticEvent e;
  e.severity = severity;
  e.where = loc;
  e.module = module.empty() ? derive_module(loc) : module;
  e.msg = fmt::format(fmtstr, std::forward<Args>(args)...);
  return e;
}

/**
 * @brief Make an Error with formatting.
 *
 * @param code Engine error code.
 * @param module Short module tag (prefer string literal/static storage).
 * @param severity Error severity (defaults to Error via wrappers).
 * @param fmtstr fmt format string.
 * @param args fmt arguments.
 * @param loc Source location (defaults to call site).
 */
template <class... Args>
[[nodiscard]] inline Error
make_error(Errc code, Severity severity, std::string_view module,
           std::source_location loc, fmt::format_string<Args...> fmtstr,
           Args &&...args) {
  Error e;
  e.code = code;
  e.severity = severity;
  e.where = loc;
  e.module = module.empty() ? derive_module(loc) : module;
  e.msg = fmt::format(fmtstr, std::forward<Args>(args)...);
  return e;
}

} // namespace util::details
