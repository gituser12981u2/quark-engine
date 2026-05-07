#include "backend/vulkan/vulkan_context.hpp"
#include "quark/utils/error_types.hpp"

#include <array>
#include <exception>
#include <quark/utils/diagnostic.hpp>
#include <quark/utils/sinks/spdlog_sink.hpp>

int main() {
  auto logger = util::make_spdlog_logger(
      {.logger_name = "quark",
       .file = {.file_path = "logs/quark.log",
                .max_bytes = 50ULL * 1024ULL * 1024ULL,
                .max_files = 5},
       .console = {.enable = true, .to_stderr = true}});
  const std::array<util::DiagnosticSink, 1> sinks = {{
      {.fn = &util::spdlog_sink, .ctx = logger.get()},
  }};
  util::set_diagnostic_sinks(sinks);

  try {
    quark::vk::VulkanContext context;

    auto s = context.run();
    if (!s) {
      util::report(s.error());
      return 1;
    }

    return 0;
  } catch (const std::exception &exception) {
    QUARK_LOG_WARN_MOD("app", "unhandled exception: {}", exception.what());
    return 2;
  }
}
