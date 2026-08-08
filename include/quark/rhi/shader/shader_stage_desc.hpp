#pragma once

#include "quark/rhi/shader/details/shader_handle.hpp"
#include "quark/rhi/shader/shader_stage.hpp"

namespace quark::rhi {

/**
 * @class ShaderStageDesc
 * @brief Describes a programmable stage used by a graphics pipeline.
 *
 * A shader stage associates a shader asset with a specific pipeline stage and
 * entry point. The referenced shader must remain alive in the shader registry
 * for the duration of pipeline creation.
 *
 * @note The stage specifies how the shader is used within the pipeline. It is
 * not inferred from the handle.
 */
struct ShaderStageDesc {
  /// Shader asset used by this stage.
  details::ShaderHandle shader{};

  /// Pipeline stage the shader participates in.
  rhi::ShaderStage stage{};

  /// Entry point within the shader module.
  const char *entry_point{"main"};
};

} // namespace quark::rhi
