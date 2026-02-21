#include <atomic>
#include <memory>
#include <quark/utils/details/diagnostic_details.hpp>
#include <quark/utils/diagnostic.hpp>
#include <quark/utils/sinks/default_sink.hpp>
#include <span>
#include <vector>

namespace util {

namespace {
using SinkList = std::vector<DiagnosticSink>;
std::atomic<std::shared_ptr<const SinkList>> g_sinks;
} // namespace

void set_diagnostic_sinks(std::span<const DiagnosticSink> sinks) noexcept {
  auto list = std::make_shared<SinkList>(sinks.begin(), sinks.end());
  g_sinks.store(std::shared_ptr<const SinkList>(std::move(list)),
                std::memory_order_release);
}

std::shared_ptr<const SinkList> diagnostic_sinks_snapshot() noexcept {
  return g_sinks.load(std::memory_order_acquire);
}

void report(const DiagnosticEvent &e) noexcept {
  auto list = diagnostic_sinks_snapshot();
  if (list && !list->empty()) {
    for (const auto &s : *list) {
      if (s.fn != nullptr) {
        s.fn(s.ctx, e);
      }
    }
    return;
  }

  default_sink(e);
}

} // namespace util
