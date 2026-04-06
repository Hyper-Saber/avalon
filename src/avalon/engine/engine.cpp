module;
#include <chrono>
#include <cstdint>
#include <debug/assert.hpp>
#include <expected>
#include <thread>

module avalon.engine;

import avalon.core;
import avalon.window;
import avalon.rhi;
import avalon.shader;
import avalon.graphics;
import avalon.physics;
import :utils;
import :application;
import avalon.input;

namespace avalon {

void Engine::Clear() {
  m_rhi->WaitIdle();
  m_userApp.Reset();
  m_scene.Reset();
  m_renderer.Reset();
  GetContext().Clear();
  m_rhi.Reset();
  m_window.Reset();

  m_defaultPipeline.Put();
}

Engine &Engine::Get() {
  static Engine instance;
  return instance;
}

constexpr StringView kWindowPluginPath = "plugins:/libavalon.window.glfw";
constexpr StringView kVkRhiPluginPath = "plugins:/libavalon.rhi.vulkan";

auto Engine::Initialize(const EngineConfig &config,
                        UniquePtr<IApplication> &&userApp)
    -> std::expected<void, EStatusCode> {
  m_config = config;
  m_userApp = std::move(userApp);

  avalon::InitializeLogger();

  auto vfs = vfs::VfsProvider::CreateVfs();
  auto device = vfs::VfsProvider::CreateDevice();
  GetContext().RegisterService<vfs::IVfs>(EEngineService::Vfs, std::move(vfs));
  auto root = vfs::FindProjectRoot(*device.Get());
  auto shaderVirtualFolderPath = Path(vfs::kShaderFolderVirtualPath);
  auto shaderFolderPath = root / vfs::kShaderFolderPath;
  vfs::GetVfs().Mount(shaderVirtualFolderPath.GetString(), shaderFolderPath,
                      device.Get());
  vfs::GetVfs().Mount("plugins:", root / vfs::kPluginFolderPath, device.Get());

  auto rhiLoadRes = LoadPlugin<rhi::IRhi>(
      {String(kVkRhiPluginPath) + platform::kPluginExtension});

  if (!rhiLoadRes) {
    return std::unexpected(EStatusCode::PluginInitializeError);
  }
  m_rhi = std::move(rhiLoadRes.value());

  uint32_t width = config.windowProps.width, height = config.windowProps.height;
  rhi::ERhiResult result;
  if (config.renderDeviceRequirement.queueRequirement.isRequirePresent) {
    auto windowLoadRes = LoadPlugin<window::IWindow>(
        {String(kWindowPluginPath) + platform::kPluginExtension});

    if (!windowLoadRes) {
      return std::unexpected(EStatusCode::PluginInitializeError);
    }
    m_window = std::move(windowLoadRes.value());
    m_window->Initialize(config.windowProps);
    m_window->GetFrameBufferSize(width, height);
    result = m_rhi->Initialize(config.renderDeviceRequirement,
                               m_window->GetNativeInfo(), width, height);
  } else {
    result = m_rhi->Initialize(config.renderDeviceRequirement);
  }

  if (result != rhi::ERhiResult::Success) {
    return std::unexpected(EStatusCode::PluginInitializeError);
  }

  m_renderer = graphics::CreateRenderer(*m_rhi.Get());

  GetContext().RegisterService<graphics::ShaderManager>(
      EEngineService::ShaderManager, *m_rhi.Get());
  GetContext().RegisterService<graphics::MeshManager>(
      EEngineService::MeshManager, *m_rhi.Get());
  GetContext().RegisterService<graphics::MaterialManager>(
      EEngineService::MaterialManager);
  GetContext().RegisterService<input::InputManager>(
      EEngineService::InputManager);

  m_defaultPipeline = MakeShared<graphics::ForwardPipeline>(*m_rhi.Get());
  m_renderer->SetPipeline(m_defaultPipeline.Get());

  m_scene = MakeUnique<scene::Scene>();

  m_scene->GetWorld().AddSystem<ecs::PhysicsSystem>();

  rhi::ProbeData data = graphics::CreateSkyboxProbeData();

  m_rhi->UpdateProbeBuffer(0, &data, sizeof(rhi::ProbeData));
  m_userApp->OnInitialize(*m_scene.Get(), *m_rhi.Get(), {width, height});
  return {};
}

void Engine::Run() {
  AVALON_ASSERT(m_window.Get() && m_rhi.Get());

  m_lastFrameTime = std::chrono::steady_clock::now();

  bool isRequestExit = false;
  while (!m_window->ShouldClose() && !isRequestExit) {
    m_window->PollEvents();
    input::GetInputManager().Update(m_window->GetInputSnapshot());

    auto currentTime = std::chrono::steady_clock::now();
    m_deltaTime =
        std::chrono::duration<float>(currentTime - m_lastFrameTime).count();
    m_lastFrameTime = currentTime;

    if constexpr (debug::kIsDebug) {
      m_deltaTime = std::min(m_deltaTime, 0.1f);
    }

    m_fpsCount++;
    m_currentFrame++;
    m_fpsTimer += m_deltaTime;

    m_totalTime += m_deltaTime;
    auto &context = GetContext();
    context.globalTime.time = m_totalTime;
    context.globalTime.sineTime = Sin(m_totalTime);
    context.globalTime.cosineTime = Cos(m_totalTime);
    context.globalTime.deltaTime = m_deltaTime;
    context.currentFrame = m_currentFrame;

    if (m_fpsTimer >= 1.f) {
      m_lastFps = m_fpsCount;

      float avgFps = static_cast<float>(m_lastFps) / m_fpsTimer;
      float avgMs = 1000.f / avgFps;

      m_fpsCount = 0;
      m_fpsTimer -= 1.f;

      String title =
          String::Format("FPS: {:.1f} | FrameTime: {:.2f} ms", avgFps, avgMs);
      m_window->SetTitle(title);
    }

    if (m_window->IsMinimized()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      continue;
    }

    if (m_window->IsResized()) {
      isRequestExit = !TryHandleRhiError(rhi::ERhiResult::SwapchainOutOfDate);
      if (isRequestExit) {
        avalon::Error("[Engine]: Exiting due to rhi error");
      }
      continue;
    }

    auto frameRes = ExecuteFrame();

    if (frameRes != rhi::ERhiResult::Success) {
      isRequestExit = !TryHandleRhiError(frameRes);
      if (isRequestExit) {
        avalon::Error("[Engine]: Exiting due to rhi error");
        continue;
      }
    }
  }
}

auto Engine::ExecuteFrame() -> rhi::ERhiResult {
  Update();
  auto beginRes = m_rhi->BeginFrame();
  if (beginRes != rhi::ERhiResult::Success) {
    TryHandleRhiError(beginRes);
    return beginRes;
  }
  m_scene->Render(*m_renderer.Get());
  return beginRes != rhi::ERhiResult::Success ? beginRes : m_rhi->EndFrame();
}

void Engine::Update() {
  m_scene->Update(m_deltaTime);
  m_userApp->OnUpdate(m_deltaTime, *m_scene.Get());
}

bool Engine::TryHandleRhiError(rhi::ERhiResult error) {
  switch (error) {
  case rhi::ERhiResult::SwapchainOutOfDate: {
    uint32_t width, height;

    m_window->GetFrameBufferSize(width, height);
    if (width == 0 || height == 0)
      break;
    Debug("[Engine]: Swapchain out of date, resizing swapchain...");
    m_renderer->OnResize(width, height);
    auto view = m_scene->GetWorld().GetView<ecs::CameraComponent>();
    view.Foreach([&](ecs::Entity entity, ecs::CameraComponent &camera) {
      camera.SetAspectRatio(width / static_cast<float>(height));
    });
    m_window->ResetResizeFlag();
    break;
  }
  case rhi::ERhiResult::OutOfMemory:
    avalon::Error("[Engine]: Out of GPU memory!");
    return false;
  case rhi::ERhiResult::DeviceLost:
    avalon::Error("[Engine]: GPU Device lost!");
    return false;
  default:
    avalon::Error("[Engine]: Unhandled RHI error!");
    return false;
  }
  return true;
}

} // namespace avalon
