#pragma once

#include <cassert>
#include <variant>

namespace quark::rhi {

struct NativeBackendRef {

  const void *ptr{nullptr};

  [[nodiscard]] bool valid() const noexcept { return ptr != nullptr; }

  template <class T> [[nodiscard]] const T *as() const noexcept {
    return static_cast<const T *>(ptr);
  }
};

template <class Variant, class F>
decltype(auto) visit_backend(const rhi::NativeBackendRef &ref, F &&fn) {
  const auto *variant = ref.as<Variant>();
  assert(variant != nullptr);
  return std::visit(std::forward<F>(fn), *variant);
}

} // namespace quark::rhi
