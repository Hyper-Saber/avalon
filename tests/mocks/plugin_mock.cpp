#include <expected>
#include <iostream>

import avalon.core;

namespace avalon::mock {

class MockPlugin : public IPlugin {
public:
  auto OnLoad() -> std::expected<void, EStatusCode> override {
    std::cout << "[Mock Plugin] OnLoad called successfully!" << std::endl;
    return {};
  }

  void Cleanup() override {
    std::cout << "[Mock Plugin] Cleanup called!" << std::endl;
  }

  void SayHello() {
    std::cout << "[Mock Plugin] Hello from the DLL!" << std::endl;
  }
};

} // namespace avalon::mock

// 导出函数
extern "C" AVALON_MOCK_PLUGIN_API avalon::IPlugin *CreatePlugin() {
  return new avalon::mock::MockPlugin();
}

extern "C" AVALON_MOCK_PLUGIN_API void DestroyPlugin(avalon::IPlugin *plugin) {
  if (plugin) {
    plugin->Cleanup();
    delete plugin;
  }
}
