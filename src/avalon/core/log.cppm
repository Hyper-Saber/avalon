module;
#include <format>
#include <string_view>
export module avalon.core:log;

export namespace avalon {

enum class LogLevel { Debug, Info, Warn, Error };
AVALON_CORE_API void LogRaw(LogLevel level, std::string_view message);
AVALON_CORE_API void InitializeLogger();

template <typename... Args>
void Info(std::format_string<Args...> fmt, Args &&...args) {
  LogRaw(LogLevel::Info,
         std::vformat(fmt.get(), std::make_format_args(args...)));
}

template <typename... Args>
void Warn(std::format_string<Args...> fmt, Args &&...args) {
  LogRaw(LogLevel::Warn,
         std::vformat(fmt.get(), std::make_format_args(args...)));
}

template <typename... Args>
void Error(std::format_string<Args...> fmt, Args &&...args) {
  LogRaw(LogLevel::Error,
         std::vformat(fmt.get(), std::make_format_args(args...)));
}

template <typename... Args>
void Debug(std::format_string<Args...> fmt, Args &&...args) {
#ifndef NDEBUG
  LogRaw(LogLevel::Debug,
         std::vformat(fmt.get(), std::make_format_args(args...)));
#endif
}

} // namespace avalon
