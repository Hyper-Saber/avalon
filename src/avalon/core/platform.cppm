module;
#include <string_view>
export module avalon.core:platform;

export namespace avalon {
#ifdef _WIN32
constexpr bool kIsWindows = true;
constexpr bool kIsLinux = false;
#elif defined(__linux__)
constexpr bool kIsWindows = false;
constexpr bool kIsLinux = true;
#endif

constexpr std::string_view kPluginExtension = AVALON_PLATFORM_DL_EXT;
} // namespace avalon
