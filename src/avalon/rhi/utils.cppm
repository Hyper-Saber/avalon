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
  switch (e) {
  case EShaderStage::Vertex:
    return "Vertex";
  case EShaderStage::Fragment:
    return "Fragment";
  case EShaderStage::Compute:
    return "Compute";
  case EShaderStage::None:
    return "None";
  case EShaderStage::All:
    return "All";
  }
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

constexpr StringView ToView(ETextureUsage usage) {
  switch (usage) {
  case ETextureUsage::None:
    return "None";
  case ETextureUsage::Sampled:
    return "Sampled";
  case ETextureUsage::ColorAttachment:
    return "ColorAttachment";
  case ETextureUsage::DepthStencilAttachment:
    return "DepthStencilAttachment";
  case ETextureUsage::Storage:
    return "Storage";
  case ETextureUsage::TransferSrc:
    return "TransferSrc";
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
  case EResourceLayout::ShaderReadOnly:
    return "ShaderReadOnly";
  case EResourceLayout::Present:
    return "Present";
  case EResourceLayout::TransferSrc:
    return "TransferSrc";
  case EResourceLayout::TransferDst:
    return "TransferDst";
    break;
  }
}

} // namespace avalon::rhi
