module;
#include <cstddef>
#include <cstdint>
#include <spirv_reflect.h>
#include <string>
export module avalon.shader:utils;

import avalon.core;
import avalon.rhi;
import :serialization;

using namespace avalon::rhi;

export namespace avalon::graphics {
auto ToView(rhi::EShaderStage stage) {
  switch (stage) {
  case rhi::EShaderStage::Vertex:
    return kDefaultVsEntryPointName;
  case rhi::EShaderStage::Fragment:
    return kDefaultFsEntryPointName;
  case rhi::EShaderStage::Compute:
    return kDefaultCsEntryPointName;
  default:
    return StringView::kEmptyView;
    ;
  }
}

auto ToESemantic(const char *semanticName) -> EVertexSemantic {
  if (!semanticName)
    return EVertexSemantic::Unknown;

  std::string_view sv(semanticName);

  if (sv.starts_with("POSITION"))
    return EVertexSemantic::Position;
  if (sv.starts_with("TEXCOORD"))
    return EVertexSemantic::TexCoord;
  if (sv.starts_with("COLOR"))
    return EVertexSemantic::Color;
  if (sv.starts_with("NORMAL"))
    return EVertexSemantic::Normal;
  // if (sv.starts_with("TANGENT"))   return EVertexSemantic::Tangent;
  // if (sv.starts_with("BINORMAL"))  return EVertexSemantic::Binormal;
  // if (sv.starts_with("BLENDWEIGHT")) return EVertexSemantic::BoneWeight;
  // if (sv.starts_with("BLENDINDICES")) return EVertexSemantic::BoneIndex;

  return EVertexSemantic::Unknown;
}

auto Utf8ToUtf16(const char *utf8Str) -> Array<char16_t> {
  Array<char16_t> result;
  if (!utf8Str)
    return result;

  const uint8_t *p = reinterpret_cast<const uint8_t *>(utf8Str);

  while (*p) {
    uint32_t cp = 0; // Unicode Code Point

    // 手动解码 UTF-8
    if ((*p & 0x80) == 0) { // 1-byte (ASCII)
      cp = *p++;
    } else if ((*p & 0xE0) == 0xC0) { // 2-bytes
      cp = (*p++ & 0x1F) << 6;
      cp |= (*p++ & 0x3F);
    } else if ((*p & 0xF0) == 0xE0) { // 3-bytes
      cp = (*p++ & 0x0F) << 12;
      cp |= (*p++ & 0x3F) << 6;
      cp |= (*p++ & 0x3F);
    } else if ((*p & 0xF8) == 0xF0) { // 4-bytes
      cp = (*p++ & 0x07) << 18;
      cp |= (*p++ & 0x3F) << 12;
      cp |= (*p++ & 0x3F) << 6;
      cp |= (*p++ & 0x3F);
    } else {
      p++; // 非法序列，跳过
      continue;
    }

    // 编码为 UTF-16 (char16_t)
    if (cp <= 0xFFFF) {
      result.PushBack(static_cast<char16_t>(cp));
    } else {
      // 处理 Surrogate Pairs (超出 16 位范围的字符，如 Emoji)
      cp -= 0x10000;
      result.PushBack(static_cast<char16_t>((cp >> 10) + 0xD800));
      result.PushBack(static_cast<char16_t>((cp & 0x3FF) + 0xDC00));
    }
  }

  result.PushBack(u'\0'); // 确保以 null 结尾，DXC 接口需要
  return result;
}

auto Utf8ToWstring(const char *utf8Str) -> std::wstring {
  if (!utf8Str)
    return L"";

  size_t len = std::mbstowcs(nullptr, utf8Str, 0);
  if (len == (size_t)-1)
    return L"";
  std::wstring result(len, L'\0');
  std::mbstowcs(&result[0], utf8Str, len);
  return result;
}

auto GetTargetProfile(EShaderStage stage, EShaderFeatureLevel level)
    -> std::wstring {
  std::wstring version = L"6_0";
  if (level == EShaderFeatureLevel::Level_6_3)
    version = L"6_3";
  else if (level == EShaderFeatureLevel::Level_6_6)
    version = L"6_6";

  switch (stage) {
  case EShaderStage::Vertex:
    return L"vs_" + version;
  case EShaderStage::Fragment:
    return L"ps_" + version;
  case EShaderStage::Compute:
    return L"cs_" + version;
  default:
    return L"lib_" + version;
  }
}

EFormat ToEFormat(const SpvReflectFormat format) {
  switch (format) {
  case SPV_REFLECT_FORMAT_UNDEFINED:
    return EFormat::Undefined;
  case SPV_REFLECT_FORMAT_R16_UINT:
    return EFormat::R16_Uint;
  case SPV_REFLECT_FORMAT_R16_SINT:
    return EFormat::R16_Int;
  case SPV_REFLECT_FORMAT_R16_SFLOAT:
    return EFormat::R16_Float;
  case SPV_REFLECT_FORMAT_R16G16_UINT:
    return EFormat::R16G16_Uint;
  case SPV_REFLECT_FORMAT_R16G16_SINT:
    return EFormat::R16G16_Int;
  case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
    return EFormat::R16G16_Float;
  case SPV_REFLECT_FORMAT_R16G16B16_UINT:
    return EFormat::R16G16B16_Uint;
  case SPV_REFLECT_FORMAT_R16G16B16_SINT:
    return EFormat::R16G16B16_Int;
  case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
    return EFormat::R16G16B16_Float;
  case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
    return EFormat::R16G16B16A16_Uint;
  case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
    return EFormat::R16G16B16A16_Int;
  case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
    return EFormat::R16G16B16A16_Float;
  case SPV_REFLECT_FORMAT_R32_UINT:
    return EFormat::R32_Uint;
  case SPV_REFLECT_FORMAT_R32_SINT:
    return EFormat::R32_Int;
  case SPV_REFLECT_FORMAT_R32_SFLOAT:
    return EFormat::R32_Float;
  case SPV_REFLECT_FORMAT_R32G32_UINT:
    return EFormat::R32G32_Uint;
  case SPV_REFLECT_FORMAT_R32G32_SINT:
    return EFormat::R32G32_Int;
  case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
    return EFormat::R32G32_Float;
  case SPV_REFLECT_FORMAT_R32G32B32_UINT:
    return EFormat::R32G32B32_Uint;
  case SPV_REFLECT_FORMAT_R32G32B32_SINT:
    return EFormat::R32G32B32_Int;
  case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
    return EFormat::R32G32B32_Float;
  case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
    return EFormat::R32G32B32A32_Uint;
  case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
    return EFormat::R32G32B32A32_Int;
  case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
    return EFormat::R32G32B32A32_Float;
  case SPV_REFLECT_FORMAT_R64_UINT:
    return EFormat::R64_Uint;
  case SPV_REFLECT_FORMAT_R64_SINT:
    return EFormat::R64_Int;
  case SPV_REFLECT_FORMAT_R64_SFLOAT:
    return EFormat::R64_Float;
  case SPV_REFLECT_FORMAT_R64G64_UINT:
    return EFormat::R64G64_Uint;
  case SPV_REFLECT_FORMAT_R64G64_SINT:
    return EFormat::R64G64_Int;
  case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
    return EFormat::R64G64_Float;
  case SPV_REFLECT_FORMAT_R64G64B64_UINT:
    return EFormat::R64G64B64_Uint;
  case SPV_REFLECT_FORMAT_R64G64B64_SINT:
    return EFormat::R64G64B64_Int;
  case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
    return EFormat::R64G64B64_Float;
  case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
    return EFormat::R64G64B64A64_Uint;
  case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
    return EFormat::R64G64B64A64_Int;
  case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
    return EFormat::R64G64B64A64_Float;
  }
}

EFormat SpvTypeToEFormat(const SpvReflectTypeDescription *type) {
  if (!type)
    return EFormat::Undefined;
  const auto &numeric = type->traits.numeric;
  const uint32_t componentCount = numeric.vector.component_count;
  const uint32_t width = numeric.scalar.width;

  if (type->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
    if (width == 16) {
      switch (componentCount) {
      case 1:
        return EFormat::R16_Float;
      case 2:
        return EFormat::R16G16_Float;
      case 3:
        return EFormat::R16G16B16_Float;
      case 4:
        return EFormat::R16G16B16A16_Float;
      };
    } else if (width == 32)
      switch (componentCount) {
      case 1:
        return EFormat::R32_Float;
      case 2:
        return EFormat::R32G32_Float;
      case 3:
        return EFormat::R32G32B32_Float;
      case 4:
        return EFormat::R32G32B32A32_Float;
      }
    else if (width == 64) {
      switch (componentCount) {
      case 1:
        return EFormat::R64_Float;
      case 2:
        return EFormat::R64G64_Float;
      case 3:
        return EFormat::R64G64B64_Float;
      case 4:
        return EFormat::R64G64B64A64_Float;
      }
    }
  } else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
    bool is_signed = type->traits.numeric.scalar.signedness != 0;
    if (width == 16) {
      switch (componentCount) {
      case 1:
        return is_signed ? EFormat::R16_Int : EFormat::R16_Uint;
      case 2:
        return is_signed ? EFormat::R16G16_Int : EFormat::R16G16_Uint;
      case 3:
        return is_signed ? EFormat::R16G16B16_Int : EFormat::R16G16B16_Uint;
      case 4:
        return is_signed ? EFormat::R16G16B16A16_Int
                         : EFormat::R16G16B16A16_Uint;
      };
    } else if (width == 32) {
      switch (componentCount) {
      case 1:
        return is_signed ? EFormat::R32_Int : EFormat::R32_Uint;
      case 2:
        return is_signed ? EFormat::R32G32_Int : EFormat::R32G32_Uint;
      case 3:
        return is_signed ? EFormat::R32G32B32_Int : EFormat::R32G32B32_Uint;
      case 4:
        return is_signed ? EFormat::R32G32B32A32_Int
                         : EFormat::R32G32B32A32_Uint;
      }
    } else if (width == 64) {
      switch (componentCount) {
      case 1:
        return is_signed ? EFormat::R64_Int : EFormat::R64_Uint;
      case 2:
        return is_signed ? EFormat::R64G64_Int : EFormat::R64G64_Uint;
      case 3:
        return is_signed ? EFormat::R64G64B64_Int : EFormat::R64G64B64_Uint;
      case 4:
        return is_signed ? EFormat::R64G64B64A64_Int
                         : EFormat::R64G64B64A64_Uint;
      }
    }
  } else if (type->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
    return EFormat::Undefined;
  }

  return EFormat::Undefined;
}
} // namespace avalon::graphics
