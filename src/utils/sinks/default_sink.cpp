#include <print>
#include <quark/utils/diagnostic.hpp>
#include <string_view>

namespace util {

namespace {

constexpr const char *severity_to_cstr_(Severity s) noexcept {
  switch (s) {
  case Severity::Info:
    return "info";
  case Severity::Warning:
    return "warn";
  case Severity::Error:
    return "error";
  }
  return "unknown";
}

} // namespace

void default_sink(const DiagnosticEvent &e) noexcept {
  const auto &w = e.where;
  const std::string_view rel = util::details::make_relative(w.file_name());

  std::println(stderr, "[{}] {} @ {}:{} ({}): {}", e.module,
               severity_to_cstr_(e.severity), rel, w.line(), w.function_name(),
               e.msg);
}

} // namespace util
