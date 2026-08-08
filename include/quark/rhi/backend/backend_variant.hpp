#pragma once

#include <variant>

namespace quark::rhi {

template <class... Stores> using BackendVariant = std::variant<Stores...>;

}
