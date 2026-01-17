module;
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

module avalon.core;

namespace avalon {

void InitializeLogger() {
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_pattern("%Y-%m-%d %H:%M:%S.%e [%^%l%$] %v");

  auto logger = std::make_shared<spdlog::logger>("avalon", console_sink);
  spdlog::set_default_logger(logger);
  spdlog::set_level(spdlog::level::debug);
}

void LogRaw(LogLevel level, std::string_view message) {
  switch (level) {
  case LogLevel::Debug:
    spdlog::debug(message);
    break;
  case LogLevel::Info:
    spdlog::info(message);
    break;
  case LogLevel::Warn:
    spdlog::warn(message);
    break;
  case LogLevel::Error:
    spdlog::error(message);
    break;
  }
}

} // namespace avalon
