module;
#include <cstdlib>
#include <source_location>
#include <stacktrace>
export module avalon.core:debug;
import :log;
import :string_view;

export namespace avalon::debug {

void inline TraceBack(uint32_t skip = 2) {
  auto trace = std::stacktrace::current(skip);
  if (!trace.empty()) {
    Debug("[Call Stack]\n{}", std::to_string(trace));
  }
}

void inline OnAssertFailed(StringView expression, StringView message,
                           const std::source_location location =
                               std::source_location::current()) noexcept {
  Debug("[Assertion Failed]\n"
        "-----------------------------------------------------------"
        "\n Condition: {}\n Message: {}\n File: {}\n Line: {}\n Function: {}\n"
        "-----------------------------------------------------------",
        expression, message, location.file_name(), location.line(),
        location.function_name());
  TraceBack();

#if defined(_MSC_VER)
  __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
  __builtin_trap();
#endif // namespace avalon::debug

  std::abort();
}

} // namespace avalon::debug
