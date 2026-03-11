#include <expected>
#include <iostream>

import test.utils;
import avalon.engine;
import avalon.core;
import avalon.rhi;

using namespace avalon;

void TestHelloTriangle() {
  EngineConfig config{
      .renderDeviceRequirement =
          {
              .queueRequirement =
                  {
                      .isRequireGraphics = true,
                      .isRequirePresent = true,
                  },
              .requiredCapabilities =
                  {
                      rhi::ERenderCapability::Swapchain,
                  },
          },
      .windowProps =
          {
              .title = "Hello Triangle Test",
              .width = 800,
              .height = 600,
          },
  };

  auto &engine = Engine::Get();
  auto initRes = engine.Initialize(config);

  test::Assert(initRes.has_value(), "Engine Initialization");

  engine.Run();

  engine.Clear();

  std::cout << "Hello Triangle Integration Test Finished Successfully!"
            << std::endl;
}

int main() {
  TestHelloTriangle();
  return 0;
}
