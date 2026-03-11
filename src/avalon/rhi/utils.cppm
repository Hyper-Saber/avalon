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
  case EFormat::R16G16_Uint2:
    return 4;
  case EFormat::R16G16_Int2:
    return 4;
  case EFormat::R16G16_Float2:
    return 4;
  case EFormat::R16G16B16_Uint3:
    return 6;
  case EFormat::R16G16B16_Int3:
    return 6;
  case EFormat::R16G16B16_Float3:
    return 6;
  case EFormat::R16G16B16A16_Uint4:
    return 8;
  case EFormat::R16G16B16A16_Int4:
    return 8;
  case EFormat::R16G16B16A16_Float4:
    return 8;
  case EFormat::R32_Uint:
    return 4;
  case EFormat::R32_Int:
    return 4;
  case EFormat::R32_Float:
    return 4;
  case EFormat::R32G32_Uint2:
    return 8;
  case EFormat::R32G32_Int2:
    return 8;
  case EFormat::R32G32_Float2:
    return 8;
  case EFormat::R32G32B32_Uint3:
    return 12;
  case EFormat::R32G32B32_Int3:
    return 12;
  case EFormat::R32G32B32_Float3:
    return 12;
  case EFormat::R32G32B32A32_Uint4:
    return 16;
  case EFormat::R32G32B32A32_Int4:
    return 16;
  case EFormat::R32G32B32A32_Float4:
    return 16;
  case EFormat::R64_Uint:
    return 8;
  case EFormat::R64_Int:
    return 8;
  case EFormat::R64_Float:
    return 8;
  case EFormat::R64G64_Uint2:
    return 16;
  case EFormat::R64G64_Int2:
    return 16;
  case EFormat::R64G64_Float2:
    return 16;
  case EFormat::R64G64B64_Uint3:
    return 24;
  case EFormat::R64G64B64_Int3:
    return 24;
  case EFormat::R64G64B64_Float3:
    return 24;
  case EFormat::R64G64B64A64_Uint4:
    return 32;
  case EFormat::R64G64B64A64_Int4:
    return 32;
  case EFormat::R64G64B64A64_Float4:
    return 32;
  default:
    break;
  }

  return kInvalidFormatSize;
}
} // namespace avalon::rhi
