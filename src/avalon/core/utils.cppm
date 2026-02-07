module;
#include <cstdint>
export module avalon.core:utils;
import :types;

export namespace avalon::rhi {
constexpr uint32_t GetFormatSize(EFormat format) {
  switch (format) {
  case EFormat::R32G32B32A32_Float4:
    return 16;
  case EFormat::R32G32B32_Float3:
    return 12;
  case EFormat::R32G32_Float2:
    return 8;
  case EFormat::R32_Float:
    return 4;
  }
}
} // namespace avalon::rhi
