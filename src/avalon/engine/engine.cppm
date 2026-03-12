module;
#include <expected>

export module avalon.engine;

import avalon.core;
import avalon.window;
import avalon.rhi;
import avalon.graphics;
import avalon.ecs;
import avalon.shader;
export import :utils;

export namespace avalon {

struct AVALON_ENGINE_API EngineConfig {
  rhi::DeviceRequirement renderDeviceRequirement;
  window::WindowProps windowProps;
};

class AVALON_ENGINE_API Engine final : NonCopyable {
public:
  static Engine &Get();

  ~Engine() = default;

  auto Initialize(const EngineConfig &config)
      -> std::expected<void, EStatusCode>;
  void Run();

  void Clear();

private:
  Engine() = default;

  auto ExecuteFrame() -> rhi::ERhiResult;
  bool TryHandleRhiError(rhi::ERhiResult error);

  void CreateTriangleEntity(const graphics::Material &material);

private:
  EngineConfig m_config;
  PluginInstance<window::IWindow> m_window;
  PluginInstance<rhi::IRhi> m_rhi;
  UniquePtr<graphics::Renderer> m_renderer;
  UniquePtr<ecs::World> m_world;
  rhi::PipelineHandle m_pipeline;
  rhi::RenderPassHandle m_renderPass;
  graphics::MeshHandle m_mesh;
};
} // namespace avalon
