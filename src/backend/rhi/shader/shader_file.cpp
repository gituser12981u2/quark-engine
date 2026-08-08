#include "quark/rhi/shader/shader_file.hpp"
#include "quark/utils/diagnostic.hpp"
#include "quark/utils/error_types.hpp"
#include "quark/utils/result.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <vector>

namespace quark::rhi {

// TODO: Cache by path
util::Result<std::vector<uint32_t>>
read_spv_file(const std::filesystem::path &path) {
  std::ifstream file{path, std::ios::binary | std::ios::ate};

  QUARK_ENSURE(file.is_open(), QUARK_ERR(util::Errc::ApiError,
                                         "failed to open SPIR-V shader file"));

  const std::streamsize size = file.tellg();

  QUARK_ENSURE(
      size > 0 && size % 4 == 0,
      QUARK_ERR(util::Errc::InvalidArg, "SPIR-V shader file size is invalid"));

  std::vector<uint32_t> code(static_cast<std::size_t>(size) / sizeof(uint32_t));

  file.seekg(0, std::ios::beg);

  QUARK_ENSURE(
      file.read(reinterpret_cast<char *>(code.data()), size),
      QUARK_ERR(util::Errc::ApiError, "failed to read SPIR-V shader file"));

  return code;
}

} // namespace quark::rhi
