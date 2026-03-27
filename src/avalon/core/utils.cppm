module;
#include <cstddef>
#include <cstdlib>
#include <cxxabi.h>
#include <type_traits>
#include <typeinfo>

export module avalon.core:utils;
import :types;
import :string_view;
import :status;
import :string;

namespace avalon::utils {
template <typename T> const String GetTypeName() noexcept {
  static String typeName = []() {
    int status;
    const char *mangled = typeid(T).name();
    char *demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    String name = (status == 0) ? demangled : mangled;
    free(demangled);
    return name;
  }();
  return typeName;
}
} // namespace avalon::utils

export namespace avalon::rhi {

template <typename T> constexpr bool HasFlag(T value, T flag) noexcept {
  using U = std::underlying_type_t<T>;
  return (static_cast<U>(value) & static_cast<U>(flag)) != 0;
}

template <typename T> struct EnableBitmaskOperators : std::false_type {};

template <typename T>
  requires EnableBitmaskOperators<T>::value
constexpr T operator|(T lhs, T rhs) noexcept {
  using U = std::underlying_type_t<T>;
  return static_cast<T>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <typename T>
  requires EnableBitmaskOperators<T>::value
constexpr T operator|=(T &lhs, T rhs) noexcept {
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
constexpr T operator&=(T &lhs, T rhs) noexcept {
  lhs = lhs & rhs;
  return lhs;
}

template <typename T>
  requires EnableBitmaskOperators<T>::value
constexpr T operator~(T value) noexcept {
  using U = std::underlying_type_t<T>;
  return static_cast<T>(~static_cast<U>(value));
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

export namespace avalon::mem {
size_t AlignUp(size_t structSize, size_t alignment) {
  if (alignment == 0)
    return structSize;
  return (structSize + alignment - 1) & ~(alignment - 1);
}
} // namespace avalon::mem
