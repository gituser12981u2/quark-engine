#pragma once

#include <quark/utils/diagnostic.hpp>

namespace util {

/**
 * @brief Default diagnostic sink.
 *
 * This is the engine's fallback sink used when no custom sinks are configured.
 * It formats the diagnostic event and emits it to stderr. It includes all
 * information stored in {@code DiagnosticEvent}.
 *
 * @param e Diagnostic event to emit
 */
void default_sink(const DiagnosticEvent &e) noexcept;

} // namespace util
