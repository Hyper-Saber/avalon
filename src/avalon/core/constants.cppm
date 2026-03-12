module;
export module avalon.core:constants;

import :string_view;

export namespace avalon::platform {
#ifdef _WIN32
constexpr bool kIsWindows = true;
constexpr bool kIsLinux = false;
#elif defined(__linux__)
constexpr bool kIsWindows = false;
constexpr bool kIsLinux = true;
#endif

constexpr StringView kPluginExtension = AVALON_PLATFORM_DL_EXT;

} // namespace avalon::platform

export namespace avalon::debug {
#ifndef NDEBUG
constexpr bool kIsDebug = true;
#else
constexpr bool kIsDebug = false;
#endif
} // namespace avalon::debug
