module;
#include <iostream>
#include <source_location>
#include <string_view>

export module test.utils;

export namespace test {
void Assert(
    bool condition, std::string_view message,
    const std::source_location location = std::source_location::current()) {
  if (!condition) {
    std::cerr << "FAILED: " << message << "\n"
              << "  File: " << location.file_name() << "\n"
              << "  Line: " << location.line() << "\n"
              << "  Function: " << location.function_name() << std::endl;
    std::exit(1);
  } else {
    std::cout << "PASSED: " << message << std::endl;
  }
}
} // namespace test
