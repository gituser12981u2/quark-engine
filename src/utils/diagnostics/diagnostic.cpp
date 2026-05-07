#include "quark/utils/error_types.hpp"
#include <atomic>
#include <memory>
#include <quark/utils/diagnostic.hpp>
#include <quark/utils/sinks/default_sink.hpp>
#include <span>
#include <utility>
#include <vector>

namespace util {

namespace {
using SinkList = std::vector<DiagnosticSink>;
std::shared_ptr<const SinkList> g_sinks;
} // namespace

// NOLINTBEGIN
void set_diagnostic_sinks(std::span<const DiagnosticSink> sinks) noexcept {
  auto list = std::make_shared<SinkList>(sinks.begin(), sinks.end());

  std::atomic_store_explicit(&g_sinks,
                             std::shared_ptr<const SinkList>(std::move(list)),
                             std::memory_order_release);
}


std::shared_ptr<const SinkList> diagnostic_sinks_snapshot() noexcept {

  return std::atomic_load_explicit(std::addressof(g_sinks), 
                                   std::memory_order_acquire);
}
// NOLINTEND
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
