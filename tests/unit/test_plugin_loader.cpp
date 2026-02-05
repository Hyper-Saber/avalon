#include <format>
#include <iostream>
#include <string_view>

import avalon.core;
import test.utils;

using namespace test_config;

namespace test_config {
static constexpr std::string_view kNonExistentPath = "missing_plugin";
static constexpr std::string_view kMockPluginName = "libmock.plugin";
} // namespace test_config

void testLoadPlugin() {
  auto result =
      avalon::LoadPlugin<avalon::IPlugin>(test_config::kNonExistentPath);

  expect(!result.has_value(), "Should fail when file does not exist");
  expect(result.error() == avalon::EStatusCode::FileNotFound,
         "Error code must be FileNotFound");
}

void test_load_valid_mock_plugin() {
  const std::string fullPath = std::format("{}{}", test_config::kMockPluginName,
                                           avalon::kPluginExtension);

  auto result = avalon::LoadPlugin<avalon::IPlugin>(fullPath);

  expect(result.has_value(), "Valid mock plugin should load successfully");
  if (result) {
    expect(result.value().get() != nullptr,
           "Plugin instance pointer should be valid");
  }
}

int main() {
  std::cout << "--- Starting Modern Avalon Unit Tests (No Macros) ---"
            << std::endl;

  testLoadPlugin();
  test_load_valid_mock_plugin();

  std::cout << "--- All Tests Completed ---" << std::endl;
  return 0;
}
