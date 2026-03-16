module;
#include <chrono>
#include <expected>
export module avalon.engine;

import avalon.core;
import avalon.window;
import avalon.rhi;
import avalon.graphics;
import avalon.ecs;
import avalon.shader;
import avalon.scene;
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

  void Update();

  bool TryHandleRhiError(rhi::ERhiResult error);

  ecs::Entity CreateGeometryEntity(const graphics::Material &material);

private:
  EngineConfig m_config;
  PluginInstance<window::IWindow> m_window;
  PluginInstance<rhi::IRhi> m_rhi;
  UniquePtr<graphics::Renderer> m_renderer;
  UniquePtr<scene::Scene> m_scene;
  rhi::PipelineHandle m_pipeline;
  rhi::RenderPassHandle m_renderPass;
  graphics::MeshHandle m_mesh;

  ecs::Entity m_model;
  ecs::Entity m_camera;

  std::chrono::steady_clock::time_point m_lastFrameTime;
  float m_deltaTime = 0.0f;

  float m_fpsTimer = 0.f;
  int m_frameCount = 0;
  int m_lastFps = 0;
};
} // namespace avalon
