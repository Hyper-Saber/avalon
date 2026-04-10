module;
#include <cstdint>
export module avalon.rhi:utils;
import :types;

export namespace avalon::rhi {

constexpr uint32_t kInvalidFormatSize = -1;

constexpr uint32_t GetFormatSize(EFormat format) {
  switch (format) {
  case EFormat::Undefined:
    return 0;
  case EFormat::R16_Uint:
    return 2;
  case EFormat::R16_Int:
    return 2;
  case EFormat::R16_Float:
    return 2;
  case EFormat::R16G16_Uint:
    return 4;
  case EFormat::R16G16_Int:
    return 4;
  case EFormat::R16G16_Float:
    return 4;
  case EFormat::R16G16B16_Uint:
    return 6;
  case EFormat::R16G16B16_Int:
    return 6;
  case EFormat::R16G16B16_Float:
    return 6;
  case EFormat::R16G16B16A16_Uint:
    return 8;
  case EFormat::R16G16B16A16_Int:
    return 8;
  case EFormat::R16G16B16A16_Float:
    return 8;
  case EFormat::R32_Uint:
    return 4;
  case EFormat::R32_Int:
    return 4;
  case EFormat::R32_Float:
    return 4;
  case EFormat::R32G32_Uint:
    return 8;
  case EFormat::R32G32_Int:
    return 8;
  case EFormat::R32G32_Float:
    return 8;
  case EFormat::R32G32B32_Uint:
    return 12;
  case EFormat::R32G32B32_Int:
    return 12;
  case EFormat::R32G32B32_Float:
    return 12;
  case EFormat::R32G32B32A32_Uint:
    return 16;
  case EFormat::R32G32B32A32_Int:
    return 16;
  case EFormat::R32G32B32A32_Float:
    return 16;
  case EFormat::R64_Uint:
    return 8;
  case EFormat::R64_Int:
    return 8;
  case EFormat::R64_Float:
    return 8;
  case EFormat::R64G64_Uint:
    return 16;
  case EFormat::R64G64_Int:
    return 16;
  case EFormat::R64G64_Float:
    return 16;
  case EFormat::R64G64B64_Uint:
    return 24;
  case EFormat::R64G64B64_Int:
    return 24;
  case EFormat::R64G64B64_Float:
    return 24;
  case EFormat::R64G64B64A64_Uint:
    return 32;
  case EFormat::R64G64B64A64_Int:
    return 32;
  case EFormat::R64G64B64A64_Float:
    return 32;
  default:
    break;
  }

  return kInvalidFormatSize;
}

constexpr bool IsBufferDescriptor(EDescriptorType type) noexcept {
  switch (type) {
  case EDescriptorType::UniformBuffer:
  case EDescriptorType::StorageBuffer:
  case EDescriptorType::UniformBufferDynamic:
  case EDescriptorType::StorageBufferDynamic:
    return true;
  default:
    return false;
  }
}

constexpr bool IsTexureDescriptor(EDescriptorType type) noexcept {
  switch (type) {
  case EDescriptorType::SampledImage:
  case EDescriptorType::StorageImage:
    return true;
  default:
    return false;
  }
}

bool IsAttachment(EResourceUsage usage) {
  return HasFlag(usage, EResourceUsage::ColorAttachment |
                            EResourceUsage::DepthStencilAttachment);
}

constexpr EResourceLayout MapUsageToLayout(EResourceUsage usage) noexcept {
  using enum EResourceUsage;
  if (usage == None)
    return EResourceLayout::Undefined;

  if (HasFlag(usage, EResourceUsage::DepthStencilAttachment) &&
      HasFlag(usage, EResourceUsage::ReadOnly)) {
    return EResourceLayout::DepthStencilReadOnly;
  }

  if (HasFlag(usage, Present))
    return EResourceLayout::Present;

  if (HasFlag(usage, DepthStencilAttachment))
    return EResourceLayout::DepthStencilAttachment;

  if (HasFlag(usage, ColorAttachment))
    return EResourceLayout::ColorAttachment;

  if (HasFlag(usage, TransferSrc))
    return EResourceLayout::TransferSrc;
  if (HasFlag(usage, TransferDst))
    return EResourceLayout::TransferDst;

  if (HasFlag(usage, ReadWrite))
    return EResourceLayout::General;

  if (HasFlag(usage, ReadOnly))
    return EResourceLayout::ShaderReadOnly;

  return EResourceLayout::Undefined;
}

constexpr bool IsWriteUsage(EResourceUsage usage) noexcept {
  using enum EResourceUsage;
  const EResourceUsage writeMask =
      ReadWrite | ColorAttachment | DepthStencilAttachment | TransferDst;
  return HasFlag(usage, writeMask);
}

constexpr EAccess MapUsageToAccess(EResourceUsage usage,
                                   bool isTexture) noexcept {
  using enum EResourceUsage;

  if (usage == None)
    return EAccess::None;

  EAccess access = EAccess::None;

  if (HasFlag(usage, Host) && HasFlag(usage, ReadOnly)) {
    return EAccess::HostRead;
  }

  if (HasFlag(usage, Host) && HasFlag(usage, ReadWrite)) {
    return EAccess::HostWrite | EAccess::HostRead;
  }

  if (HasFlag(usage, TransferSrc))
    return EAccess::TransferRead;
  if (HasFlag(usage, TransferDst))
    return EAccess::TransferWrite;

  if (isTexture) {
    if (HasFlag(usage, ReadOnly) && !IsAttachment(usage))
      return EAccess::TextureRead;
    if (HasFlag(usage, ReadWrite))
      return EAccess::StorageRead | EAccess::StorageWrite;

    if (HasFlag(usage, ColorAttachment)) {
      return EAccess::ColorRead | EAccess::ColorWrite;
    }
    if (HasFlag(usage, DepthStencilAttachment)) {
      return EAccess::DepthStencilRead | EAccess::DepthStencilWrite;
    }
  } else {
    if (HasFlag(usage, VertexBuffer))
      access |= EAccess::VertexAttributeRead;
    if (HasFlag(usage, IndexBuffer))
      access |= EAccess::IndexRead;
    if (HasFlag(usage, IndirectBuffer))
      access |= EAccess::IndirectCommandRead;

    if (HasFlag(usage, UniformBuffer))
      return access | EAccess::UniformRead;

    if (HasFlag(usage, ReadOnly))
      return access | EAccess::StorageRead;
    if (HasFlag(usage, ReadWrite))
      return access | EAccess::StorageRead | EAccess::StorageWrite;
  }

  return access;
}

constexpr EPipelineStage MapUsageToStage(EResourceUsage usage,
                                         EShaderStage shaderStage) noexcept {
  using enum EResourceUsage;

  if (usage == None)
    return EPipelineStage::None;
  EPipelineStage stage = EPipelineStage::None;

  if (HasFlag(usage, EResourceUsage::Host)) {
    return EPipelineStage::Host;
  }

  if (HasFlag(usage, TransferSrc | TransferDst)) {
    return EPipelineStage::Transfer;
  }

  if (HasFlag(usage, Present)) {
    return EPipelineStage::BottomOfPipe;
  }

  if (HasFlag(usage, DepthStencilAttachment)) {
    return (EPipelineStage::EarlyFragmentTests |
            EPipelineStage::LateFragmentTests);
  }

  if (HasFlag(usage, ColorAttachment)) {
    return EPipelineStage::ColorAttachmentOutput;
  }

  if (HasFlag(usage, IndirectBuffer))
    return EPipelineStage::DrawIndirect;

  if (HasFlag(usage, VertexBuffer | IndexBuffer))
    stage |= EPipelineStage::VertexInput;

  if (!IsAttachment(usage) && HasFlag(usage, ReadOnly | ReadWrite)) {
    if (HasFlag(shaderStage, EShaderStage::Vertex))
      stage |= EPipelineStage::VertexShader;
    else if (HasFlag(shaderStage, EShaderStage::Fragment))
      stage |= EPipelineStage::FragmentShader;
    else if (HasFlag(shaderStage, EShaderStage::Compute))
      stage |= EPipelineStage::ComputeShader;
  }

  return stage;
}

constexpr StringView ToView(EFormat format) {
  using enum EFormat;
  switch (format) {
  case Undefined:
    return "Undefined";

  // 16-bit
  case R16_Uint:
    return "R16_Uint";
  case R16_Int:
    return "R16_Int";
  case R16_Float:
    return "R16_Float";
  case R16G16_Uint:
    return "R16G16_Uint";
  case R16G16_Int:
    return "R16G16_Int";
  case R16G16_Float:
    return "R16G16_Float";
  case R16G16B16_Uint:
    return "R16G16B16_Uint";
  case R16G16B16_Int:
    return "R16G16B16_Int";
  case R16G16B16_Float:
    return "R16G16B16_Float";
  case R16G16B16A16_Uint:
    return "R16G16B16A16_Uint";
  case R16G16B16A16_Int:
    return "R16G16B16A16_Int";
  case R16G16B16A16_Float:
    return "R16G16B16A16_Float";

  // 32-bit
  case R32_Uint:
    return "R32_Uint";
  case R32_Int:
    return "R32_Int";
  case R32_Float:
    return "R32_Float";
  case R32G32_Uint:
    return "R32G32_Uint";
  case R32G32_Int:
    return "R32G32_Int";
  case R32G32_Float:
    return "R32G32_Float";
  case R32G32B32_Uint:
    return "R32G32B32_Uint";
  case R32G32B32_Int:
    return "R32G32B32_Int";
  case R32G32B32_Float:
    return "R32G32B32_Float";
  case R32G32B32A32_Uint:
    return "R32G32B32A32_Uint";
  case R32G32B32A32_Int:
    return "R32G32B32A32_Int";
  case R32G32B32A32_Float:
    return "R32G32B32A32_Float";

  // 64-bit
  case R64_Uint:
    return "R64_Uint";
  case R64_Int:
    return "R64_Int";
  case R64_Float:
    return "R64_Float";
  case R64G64_Uint:
    return "R64G64_Uint";
  case R64G64_Int:
    return "R64G64_Int";
  case R64G64_Float:
    return "R64G64_Float";
  case R64G64B64_Uint:
    return "R64G64B64_Uint";
  case R64G64B64_Int:
    return "R64G64B64_Int";
  case R64G64B64_Float:
    return "R64G64B64_Float";
  case R64G64B64A64_Uint:
    return "R64G64B64A64_Uint";
  case R64G64B64A64_Int:
    return "R64G64B64A64_Int";
  case R64G64B64A64_Float:
    return "R64G64B64A64_Float";

  // Packed / Normalized / Special
  case R8G8B8_UNORM:
    return "R8G8B8_UNORM";
  case R8G8B8A8_UNORM:
    return "R8G8B8A8_UNORM";
  case R8G8B8_SRGB:
    return "R8G8B8_SRGB";
  case R8G8B8A8_SRGB:
    return "R8G8B8A8_SRGB";
  case B8G8R8A8_SRGB:
    return "B8G8R8A8_SRGB";
  case R16G16_SFLOAT:
    return "R16G16_SFLOAT";
  case R16G16B16A16_SFLOAT:
    return "R16G16B16A16_SFLOAT";
  case D32_SFLOAT:
    return "D32_SFLOAT";
  case D32_SFLOAT_S8_UINT:
    return "D32_SFLOAT_S8_UINT";

  default:
    return "UnknownFormat";
  }
}

constexpr bool IsDepthFormat(EFormat format) {
  return format == EFormat::D32_SFLOAT || format == EFormat::D32_SFLOAT_S8_UINT;
}

bool HasStencilComponent(EFormat format) {
  return format == EFormat::D32_SFLOAT_S8_UINT;
}

constexpr StringView ToView(ESampleCount e) {
  switch (e) {
  case ESampleCount::SampleCount1x:
    return "1x";
  case ESampleCount::SampleCount2x:
    return "2x";
  case ESampleCount::SampleCount4x:
    return "4x";
  case ESampleCount::SampleCount8x:
    return "8x";
  case ESampleCount::SampleCount16x:
    return "16x";
  default:
    return "Unknown";
  }
}

constexpr StringView ToView(EDescriptorType e) {
  switch (e) {
  case EDescriptorType::UniformBuffer:
    return "UniformBuffer";
  case EDescriptorType::StorageBuffer:
    return "StorageBuffer";
  case EDescriptorType::UniformBufferDynamic:
    return "UniformBufferDynamic";
  case EDescriptorType::CombinedImageSampler:
    return "CombinedImageSampler";
  case EDescriptorType::SampledImage:
    return "SampledImage";
  case EDescriptorType::Sampler:
    return "Sampler";
  case EDescriptorType::StorageImage:
    return "StorageImage";
  case EDescriptorType::UniformTexelBuffer:
    return "UniformTexelBuffer";
  case EDescriptorType::StorageTexelBuffer:
    return "StorageTexelBuffer";
  case EDescriptorType::StorageBufferDynamic:
    return "StorageBufferDynamic";
  case EDescriptorType::InputAttachment:
    return "InputAttachment";
  case EDescriptorType::AccelerationStructure:
    return "AccelerationStructure";
  }
}

constexpr StringView ToView(EAttachmentIntent e) {
  if (e == EAttachmentIntent::None) {
    return "None";
  }

  if (e == EAttachmentIntent::WriteColor) {
    return "WriteColor";
  }
  if (e == EAttachmentIntent::WriteDepth) {
    return "WriteDepth";
  }
  if (e == EAttachmentIntent::CaptureSource) {
    return "CaptureSource";
  }
  if (e == EAttachmentIntent::ComputeStorage) {
    return "ComputeStorage";
  }
  if (e == EAttachmentIntent::ReadOnly) {
    return "ReadOnly";
  }

  // 处理复杂或非常规组合
  return "Composite Intent";
}

constexpr StringView ToView(ERhiResult e) {
  switch (e) {
  case ERhiResult::Success:
    return "Success";
  case ERhiResult::Unknown:
    return "Unknown";
  case ERhiResult::InitializationFailed:
    return "InitializationFailed";
  case ERhiResult::SurfaceLost:
    return "SurfaceLost";
  case ERhiResult::DeviceLost:
    return "DeviceLost";
  case ERhiResult::OutOfMemory:
    return "OutOfMemory";
  case ERhiResult::BackendSpecificError:
    return "BackendSpecificError";
  case ERhiResult::SwapchainOutOfDate:
    return "SwapchainOutOfDate";
  case ERhiResult::FailedToRecordCommand:
    return "FailedToRecordCommand";
  case ERhiResult::FailedToSubmitQueue:
    return "FailedToSubmitQueue";
  case ERhiResult::FormatNotSupported:
    return "FormatNotSupported";
  }
}

constexpr StringView ToView(EShaderStage e) {
  if (e == EShaderStage::Vertex) {
    return "Vertex";
  }
  if (e == EShaderStage::Fragment) {
    return "Fragment";
  }
  if (e == EShaderStage::Compute) {
    return "Compute";
  }
  if (e == (EShaderStage::Vertex | EShaderStage::Fragment)) {
    return "Vertex | Fragment";
  }
  if (e == (EShaderStage::Vertex | EShaderStage::Compute)) {
    return "Vertex | Compute";
  }
  if (e == (EShaderStage::Fragment | EShaderStage::Compute)) {
    return "Fragment | Compute";
  }
  if (e == EShaderStage::All) {
    return "All";
  }
  return "Unknown";
}

constexpr StringView ToView(EAttachmentLoadOp op) {
  switch (op) {
  case EAttachmentLoadOp::Load:
    return "Load";
  case EAttachmentLoadOp::Clear:
    return "Clear";
  case EAttachmentLoadOp::DontCare:
    return "DontCare";
    break;
  }
}

constexpr String ToView(EResourceUsage usage) {
  if (usage == EResourceUsage::None)
    return "None";

  String result = "";
  auto AddFlag = [&](EResourceUsage flag, const char *name) {
    if (HasFlag(usage, flag)) {
      if (!result.IsEmpty())
        result += " | ";
      result += name;
    }
  };

  AddFlag(EResourceUsage::VertexBuffer, "VertexBuffer");
  AddFlag(EResourceUsage::IndexBuffer, "IndexBuffer");
  AddFlag(EResourceUsage::IndirectBuffer, "IndirectBuffer");
  AddFlag(EResourceUsage::UniformBuffer, "UniformBuffer");
  AddFlag(EResourceUsage::StorageBuffer, "StorageBuffer");
  AddFlag(EResourceUsage::ReadOnly, "ReadOnly");
  AddFlag(EResourceUsage::ReadWrite, "ReadWrite");
  AddFlag(EResourceUsage::ColorAttachment, "ColorAttachment");
  AddFlag(EResourceUsage::DepthStencilAttachment, "DepthStencilAttachment");
  AddFlag(EResourceUsage::TransferSrc, "TransferSrc");
  AddFlag(EResourceUsage::TransferDst, "TransferDst");
  AddFlag(EResourceUsage::Present, "Present");
  AddFlag(EResourceUsage::SceneGlobals, "SceneGlobals");

  return result.IsEmpty() ? "Unknown" : result;
}

constexpr String ToView(EPipelineStage stage) {
  if (stage == EPipelineStage::None)
    return "None";

  String result = "";
  auto AddFlag = [&](EPipelineStage flag, const char *name) {
    if (HasFlag(stage, flag)) {
      if (!result.IsEmpty())
        result += " | ";
      result += name;
    }
  };

  AddFlag(EPipelineStage::TopOfPipe, "TopOfPipe");
  AddFlag(EPipelineStage::BottomOfPipe, "BottomOfPipe");
  AddFlag(EPipelineStage::DrawIndirect, "DrawIndirect");
  AddFlag(EPipelineStage::VertexInput, "VertexInput");
  AddFlag(EPipelineStage::VertexShader, "VertexShader");
  AddFlag(EPipelineStage::TessControlShader, "TessControlShader");
  AddFlag(EPipelineStage::TessEvaluationShader, "TessEvaluationShader");
  AddFlag(EPipelineStage::GeometryShader, "GeometryShader");
  AddFlag(EPipelineStage::FragmentShader, "FragmentShader");
  AddFlag(EPipelineStage::ComputeShader, "ComputeShader");
  AddFlag(EPipelineStage::EarlyFragmentTests, "EarlyFragmentTests");
  AddFlag(EPipelineStage::LateFragmentTests, "LateFragmentTests");
  AddFlag(EPipelineStage::ColorAttachmentOutput, "ColorAttachmentOutput");
  AddFlag(EPipelineStage::Transfer, "Transfer");
  AddFlag(EPipelineStage::Clear, "Clear");
  AddFlag(EPipelineStage::Host, "Host");
  AddFlag(EPipelineStage::AllGraphics, "AllGraphics");
  AddFlag(EPipelineStage::AllCommands, "AllCommands");
  AddFlag(EPipelineStage::RayTracingShader, "RayTracingShader");

  return result.IsEmpty() ? "Unknown" : result;
}

constexpr String ToView(EAccess access) {
  if (access == EAccess::None)
    return "None";

  String result = "";
  auto AddFlag = [&](EAccess flag, const char *name) {
    if (HasFlag(access, flag)) {
      if (!result.IsEmpty())
        result += " | ";
      result += name;
    }
  };

  AddFlag(EAccess::ColorRead, "ColorRead");
  AddFlag(EAccess::ColorWrite, "ColorWrite");
  AddFlag(EAccess::DepthStencilRead, "DepthStencilRead");
  AddFlag(EAccess::DepthStencilWrite, "DepthStencilWrite");
  AddFlag(EAccess::TextureRead, "TextureRead");
  AddFlag(EAccess::TransferRead, "TransferRead");
  AddFlag(EAccess::TransferWrite, "TransferWrite");
  AddFlag(EAccess::MemoryRead, "MemoryRead");
  AddFlag(EAccess::MemoryWrite, "MemoryWrite");
  AddFlag(EAccess::IndirectCommandRead, "IndirectCommandRead");
  AddFlag(EAccess::IndexRead, "IndexRead");
  AddFlag(EAccess::VertexAttributeRead, "VertexAttributeRead");
  AddFlag(EAccess::UniformRead, "UniformRead");
  AddFlag(EAccess::StorageRead, "StorageRead");
  AddFlag(EAccess::StorageWrite, "StorageWrite");
  AddFlag(EAccess::HostWrite, "HostWrite");
  AddFlag(EAccess::HostRead, "HostRead");

  return result.IsEmpty() ? "Unknown" : result;
}

constexpr StringView ToView(EAttachmentStoreOp op) {
  switch (op) {
  case EAttachmentStoreOp::Store:
    return "Store";
  case EAttachmentStoreOp::DontCare:
    return "DontCare";
    break;
  }
}

constexpr StringView ToView(EResourceLayout e) {
  switch (e) {
  case EResourceLayout::Undefined:
    return "Undefined";
  case EResourceLayout::ColorAttachment:
    return "ColorAttachment";
  case EResourceLayout::DepthStencilAttachment:
    return "DepthStencilAttachment";
  case EResourceLayout::DepthStencilReadOnly:
    return "DepthStencilReadOnly";
  case EResourceLayout::ShaderReadOnly:
    return "ShaderReadOnly";
  case EResourceLayout::Present:
    return "Present";
  case EResourceLayout::TransferSrc:
    return "TransferSrc";
  case EResourceLayout::TransferDst:
    return "TransferDst";
  case EResourceLayout::General:
    return "General";
  }
}
constexpr StringView ToView(EPrimitiveTopology e) noexcept {
  switch (e) {
  case EPrimitiveTopology::PointList:
    return "PointList";
  case EPrimitiveTopology::LineList:
    return "LineList";
  case EPrimitiveTopology::TriangleList:
    return "TriangleList";
  }
  return "Unknown";
}

constexpr StringView ToView(EPolygonMode e) noexcept {
  switch (e) {
  case EPolygonMode::Fill:
    return "Fill";
  case EPolygonMode::Line:
    return "Line";
  case EPolygonMode::Point:
    return "Point";
  }
  return "Unknown";
}

constexpr StringView ToView(ECullMode e) noexcept {
  switch (e) {
  case ECullMode::Back:
    return "Back";
  case ECullMode::Front:
    return "Front";
  case ECullMode::FrontAndBack:
    return "FrontAndBack";
  case ECullMode::None:
    return "None";
  }
  return "Unknown";
}

constexpr StringView ToView(ECompareOp e) noexcept {
  switch (e) {
  case ECompareOp::Less:
    return "Less";
  case ECompareOp::LessOrEqual:
    return "LessOrEqual";
  case ECompareOp::Greater:
    return "Greater";
  case ECompareOp::GreaterOrEqual:
    return "GreaterOrEqual";
  case ECompareOp::Equal:
    return "Equal";
  case ECompareOp::NotEqual:
    return "NotEqual";
  case ECompareOp::Always:
    return "Always";
  }
  return "Unknown";
}

constexpr StringView ToView(EFrontFace e) noexcept {
  switch (e) {
  case EFrontFace::Clockwise:
    return "Clockwise";
  case EFrontFace::CounterClockwise:
    return "CounterClockwise";
    break;
  }
}

String InputAssemblyState::ToString() const {
  return String::Format(
      "InputAssemblyState{{topology = {}, primitiveRestartEnable = {}}}",
      ToView(topology), primitiveRestartEnable);
}

String RasterizationState::ToString() const {
  return String::Format(
      "RasterizationState{{polygonMode = {}, cullMode = {}, frontFace = {}, "
      "lineWidth = {}, depthBiasEnable = {}}}",
      ToView(polygonMode), ToView(cullMode), ToView(frontFace), lineWidth,
      depthBiasEnable);
}

String DepthStencilState::ToString() const {
  return String::Format(
      "DepthStencilState{{isDepthTestEnable = {}, isDepthWriteEnable = {}, "
      "depthCompareOp = {}, isStencilTestEnable = {}}}",
      isDepthTestEnable, isDepthWriteEnable, ToView(depthCompareOp),
      isStencilTestEnable);
}
} // namespace avalon::rhi
