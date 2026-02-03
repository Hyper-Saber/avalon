export module avalon.core:platform;

export namespace avalon {
#ifdef _WIN32
constexpr bool is_windows = true;
constexpr bool is_linux = false;
#elif defined(__linux__)
constexpr bool kIsWindows = false;
constexpr bool kIsLinux = true;
#endif
} // namespace avalon
