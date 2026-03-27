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
export import :application;
export import :utils;

export namespace avalon::vfs {
constexpr StringView kShaderFolderPath = "/tests/shaders/";
constexpr StringView kPluginFolderPath = "/build/linux/x86_64/debug/plugins/";
} // namespace avalon::vfs

export namespace avalon {

struct AVALON_ENGINE_API EngineConfig {
  rhi::DeviceRequirement renderDeviceRequirement;
  window::WindowProps windowProps;
};

class AVALON_ENGINE_API Engine final : NonCopyable {
public:
  static Engine &Get();

  ~Engine() = default;

  auto Initialize(const EngineConfig &config, UniquePtr<IApplication> &&userApp)
      -> std::expected<void, EStatusCode>;

  float GetTotalTime() const { return m_totalTime; }

  auto GetRenderer() const -> graphics::IRenderer & {
    return *m_renderer.Get();
  }

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
  UniquePtr<graphics::IRenderer> m_renderer;
  UniquePtr<scene::Scene> m_scene;
  UniquePtr<IApplication> m_userApp;

  RefCountedPtr<graphics::IRenderPipeline> m_defaultPipeline;

  std::chrono::steady_clock::time_point m_lastFrameTime;
  float m_deltaTime = 0.0f;
  float m_totalTime = 0.0f;
  float m_fpsTimer = 0.f;
  uint64_t m_currentFrame = 0;
  uint32_t m_fpsCount = 0;
  uint32_t m_lastFps = 0;
};
} // namespace avalon
