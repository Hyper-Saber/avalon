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

constexpr EAccess MapUsageToAccess(EResourceUsage usage) noexcept {
  using enum EResourceUsage;

  if (usage == None)
    return EAccess::None;

  EAccess access = EAccess::None;

  if (HasFlag(usage, VertexBuffer))
    access |= EAccess::MemoryRead;
  if (HasFlag(usage, IndexBuffer))
    access |= EAccess::MemoryRead;
  if (HasFlag(usage, IndirectBuffer))
    access |= EAccess::MemoryRead;

  if (HasFlag(usage, UniformBuffer))
    access |= EAccess::ShaderRead;
  if (HasFlag(usage, ReadOnly))
    access |= EAccess::ShaderRead;
  if (HasFlag(usage, ReadWrite))
    access |= (EAccess::ShaderRead | EAccess::ShaderWrite);

  if (HasFlag(usage, ColorAttachment)) {
    access |= (EAccess::ColorRead | EAccess::ColorWrite);
  }
  if (HasFlag(usage, DepthStencilAttachment)) {
    access |= (EAccess::DepthStencilRead | EAccess::DepthStencilWrite);
  }

  if (HasFlag(usage, TransferSrc))
    access |= EAccess::TransferRead;
  if (HasFlag(usage, TransferDst))
    access |= EAccess::TransferWrite;

  if (HasFlag(usage, Present))
    access |= EAccess::None;

  return access;
}

constexpr EPipelineStage MapUsageToStage(EResourceUsage usage) noexcept {
  using enum EResourceUsage;

  if (usage == None)
    return EPipelineStage::None;

  EPipelineStage stage = EPipelineStage::None;

  if (HasFlag(usage, IndirectBuffer))
    stage |= EPipelineStage::DrawIndirect;

  if (HasFlag(usage, VertexBuffer | IndexBuffer))
    stage |= EPipelineStage::VertexInput;

  if (HasFlag(usage, UniformBuffer | ReadOnly | ReadWrite)) {
    stage |= (EPipelineStage::VertexShader | EPipelineStage::FragmentShader |
              EPipelineStage::ComputeShader);
  }

  if (HasFlag(usage, ColorAttachment)) {
    stage |= EPipelineStage::ColorAttachmentOutput;
  }

  if (HasFlag(usage, DepthStencilAttachment)) {
    stage |= (EPipelineStage::EarlyFragmentTests |
              EPipelineStage::LateFragmentTests);
  }

  if (HasFlag(usage, TransferSrc | TransferDst)) {
    stage |= EPipelineStage::Transfer;
  }

  if (HasFlag(usage, Present)) {
    stage |= EPipelineStage::BottomOfPipe;
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

constexpr StringView ToView(EResourceUsage usage) {
  switch (usage) {
  case EResourceUsage::None:
    return "None";
  case EResourceUsage::VertexBuffer:
    return "VertexBuffer";
  case EResourceUsage::IndexBuffer:
    return "IndexBuffer";
  case EResourceUsage::IndirectBuffer:
    return "IndirectBuffer";
  case EResourceUsage::UniformBuffer:
    return "UniformBuffer";
  case EResourceUsage::StorageBuffer:
    return "StorageBuffer";
  case EResourceUsage::ReadOnly:
    return "ReadOnly";
  case EResourceUsage::ReadWrite:
    return "ReadWrite";
  case EResourceUsage::ColorAttachment:
    return "ColorAttachment";
  case EResourceUsage::DepthStencilAttachment:
    return "DepthStencilAttachment";
  case EResourceUsage::TransferSrc:
    return "TransferSrc";
  case EResourceUsage::TransferDst:
    return "TransferDst";
  case EResourceUsage::Present:
    return "Present";
    break;
  }
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
