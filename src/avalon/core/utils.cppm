module;
#include <type_traits>

export module avalon.core:utils;
import :types;
import :string_view;
import :status;

export namespace avalon::rhi {

template <typename T> struct EnableBitmaskOperators : std::false_type {};

template <typename T>
  requires EnableBitmaskOperators<T>::value
constexpr T operator|(T lhs, T rhs) noexcept {
  using U = std::underlying_type_t<T>;
  return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <typename T>
  requires EnableBitmaskOperators<T>::value
constexpr T operator|=(T lhs, T rhs) noexcept {
  lhs = lhs | rhs;
  return lhs;
}

template <typename T>
  requires EnableBitmaskOperators<T>::value
constexpr T operator&(T lhs, T rhs) noexcept {
  using U = std::underlying_type_t<T>;
  return static_cast<T>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <typename T>
  requires EnableBitmaskOperators<T>::value
constexpr T operator&=(T lhs, T rhs) noexcept {
  lhs = lhs & rhs;
  return lhs;
}

} // namespace avalon::rhi
//
export namespace avalon {
auto EstatusCodeToView(EStatusCode code) {
  switch (code) {
  case EStatusCode::Success:
    return "Success";
  case EStatusCode::WindowError:
    return "WindowError";
  case EStatusCode::SymbolNotFound:
    return "SymbolNotFound";
  case EStatusCode::PluginInitializeError:
    return "PluginInitializeError";
  case EStatusCode::FileNotFound:
    return "FileNotFound";
  case EStatusCode::RhiError:
    return "RhiError";
  case EStatusCode::OutOfMemory:
    return "OutOfMemory";
  case EStatusCode::DeviceLost:
    return "DeviceLost";
  case EStatusCode::RhiUpdateFailed:
    return "RhiUpdateFailed";
  case EStatusCode::InvalidParameter:
    return "InvalidParameter";
  case EStatusCode::NotSupported:
    return "NotSupported";
  case EStatusCode::InternalError:
    return "InternalError";
    break;
  }
}
} // namespace avalon
