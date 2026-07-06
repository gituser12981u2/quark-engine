#pragma once

#include "quark/utils/result.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace quark::rhi {

[[nodiscard]] util::Result<std::vector<uint32_t>>
read_spv_file(const std::filesystem::path &path);

}
