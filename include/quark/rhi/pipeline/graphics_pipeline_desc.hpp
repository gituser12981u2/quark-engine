#pragma once

#include "quark/rhi/pipeline/vertex_layout.hpp"
#include "quark/rhi/shader/shader_stage_desc.hpp"

#include <cstdint>
#include <span>

namespace quark::rhi {

class PipelineLayout;

enum class PrimitiveTopologyClass : uint8_t {
  Point,
  Line,
  Triangle,
  Patch,
};

enum class PrimitiveTopology : uint8_t {
  PointList,

  LineList,
  LineStrip,

  TriangleList,
  TriangleStrip,
};

enum class IndexStripCut : uint8_t {
  Disabled,
  Value0xFFFF,
  Value0xFFFFFFFF,
};

enum class FillMode : uint8_t {
  Solid,
  Wireframe,
};

enum class CullMode : uint8_t {
  None,
  Front,
  Back,
};

enum class FrontFace : uint8_t {
  CounterClockwise,
  Clockwise,
};

enum class CompareOp : uint8_t {
  Never,
  Less,
  Equal,
  LessOrEqual,
  Greater,
  NotEqual,
  GreaterOrEqual,
  Always,
};

enum class SampleCount : uint8_t {
  Count1 = 1,
  Count2 = 2,
  Count4 = 4,
  Count8 = 8,
  Count16 = 16,
};

enum class BlendFactor : uint8_t {
  Zero,
  One,

  SourceColor,
  OneMinusSourceColor,
  DestinationColor,
  OneMinusDestinationColor,

  SourceAlpha,
  OneMinusSourceAlpha,
  DestinationAlpha,
  OneMinusDestinationAlpha,

  ConstantColor,
  OneMinusConstantColor,
  ConstantAlpha,
  OneMinusConstantAlpha,

  SourceAlphaSaturate,
};

enum class BlendOp : uint8_t {
  Add,
  Subtract,
  ReverseSubtract,
  Min,
  Max,
};

enum class ColorWriteMask : uint8_t {
  None = 0,
  Red = 1U << 0U,
  Green = 1U << 1U,
  Blue = 1U << 2U,
  Alpha = 1U << 3U,
  All = 0x0F,
};

struct ColorAttachmentDesc {
  Format format{Format::Undefined};

  bool blend_enable{false};

  BlendFactor source_color_factor{BlendFactor::One};
  BlendFactor destination_color_factor{BlendFactor::Zero};
  BlendOp color_op{BlendOp::Add};

  BlendFactor source_alpha_factor{BlendFactor::One};
  BlendFactor destination_alpha_factor{BlendFactor::Zero};
  BlendOp alpha_op{BlendOp::Add};

  ColorWriteMask write_mask{ColorWriteMask::All};
};

struct DepthAttachmentDesc {
  Format format{Format::Undefined};

  bool test_enable{false};
  bool write_enable{false};
  CompareOp compare_op{CompareOp::Less};
};

struct RasterStateDesc {
  FillMode fill_mode{FillMode::Solid};
  CullMode cull_mode{CullMode::Back};
  FrontFace front_face{FrontFace::CounterClockwise};

  bool depth_clip_enable{true};

  int32_t depth_bias{0};
  float depth_bias_clamp{0.0F};
  float slope_scaled_depth_bias{0.0F};
};

struct MultisampleStateDesc {
  SampleCount samples{SampleCount::Count1};
  bool alpha_to_coverage_enable{false};
};

struct GraphicsPipelineDesc {
  std::span<const ShaderStageDesc> stages;

  VertexLayoutDesc vertex_layout{};

  PrimitiveTopologyClass topology_class{PrimitiveTopologyClass::Triangle};

  IndexStripCut index_strip_cut{IndexStripCut::Disabled};

  RasterStateDesc raster{};
  MultisampleStateDesc multisample{};

  std::span<const ColorAttachmentDesc> color_attachments;
  DepthAttachmentDesc depth_attachment{};

  const PipelineLayout *pipeline_layout{nullptr};
};

} // namespace quark::rhi
