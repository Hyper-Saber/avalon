module;
#include <chrono>
#include <cmath>
#include <cstdint>
#include <debug/assert.hpp>
#include <expected>

module avalon.engine;

import avalon.core;
import avalon.window;
import avalon.rhi;
import avalon.shader;
import avalon.graphics;
import :utils;
import :application;

namespace avalon {

void Engine::Clear() {
  m_rhi.Reset();
  m_window.Reset();
}

Engine &Engine::Get() {
  static Engine instance;
  return instance;
}

constexpr StringView kWindowPluginPath = "plugins/libavalon.window.glfw";
constexpr StringView kVkRhiPluginPath = "plugins/libavalon.rhi.vulkan";

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
  auto shaderVirtualFolderPath = Path(kShaderFolderVirtualPath);
  auto shaderFolderPath = root / kShaderFolderPath;
  vfs::GetVfs().Mount(shaderVirtualFolderPath.GetString(), shaderFolderPath,
                      device.Get());

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

  GetContext().RegisterService<graphics::ShaderManager>(
      EEngineService::ShaderManager, *m_rhi.Get());
  GetContext().RegisterService<graphics::MeshManager>(
      EEngineService::MeshManager, *m_rhi.Get());

  m_scene = MakeUnique<scene::Scene>();
  m_userApp->OnInitialize(*m_scene.Get(), *m_rhi.Get(), {width, height});
  return {};
}

void Engine::Run() {
  AVALON_ASSERT(m_window.Get() && m_rhi.Get());

  m_lastFrameTime = std::chrono::steady_clock::now();

  bool isRequestExit = false;
  while (!m_window->ShouldClose() && !isRequestExit) {
    m_window->PollEvents();

    auto currentTime = std::chrono::steady_clock::now();
    m_deltaTime =
        std::chrono::duration<float>(currentTime - m_lastFrameTime).count();
    m_lastFrameTime = currentTime;

    if constexpr (debug::kIsDebug) {
      m_deltaTime = std::min(m_deltaTime, 0.1f);
    }

    m_frameCount++;
    m_fpsTimer += m_deltaTime;

    if (m_fpsTimer >= 1.f) {
      m_lastFps = m_frameCount;

      float avgFps = static_cast<float>(m_lastFps) / m_fpsTimer;
      float avgMs = 1000.f / avgFps;

      String title =
          String::Format("FPS: {:.1f} | FrameTime: {:.2f} ms", avgFps, avgMs);
      m_window->SetTitle(title);
    }

    if (m_window->IsMinimized())
      continue;

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
  auto cmd = m_rhi->GetMainCommandBuffer();
  cmd->Begin();
  m_scene->Render(*m_renderer.Get(), *cmd);
  cmd->End();
  m_rhi->Submit(cmd);
  return beginRes != rhi::ERhiResult::Success ? beginRes : m_rhi->EndFrame();
}

void Engine::Update() {
  m_totalTime += m_deltaTime;

  m_scene->GetWorld().Update(m_deltaTime);

  m_userApp->OnUpdate(m_deltaTime, *m_scene.Get());
}

bool Engine::TryHandleRhiError(rhi::ERhiResult error) {
  switch (error) {
  case rhi::ERhiResult::SwapchainOutOfDate: {
    uint32_t width, height;

    m_window->GetFrameBufferSize(width, height);
    if (width == 0 || height == 0)
      break;
    avalon::Info("[Engine]: Swapchain out of date, resizing swapchain...");
    auto res = m_rhi->RecreateSwapchain(m_renderPass, width, height);
    m_renderer->OnResize({width, height});
    auto view = m_scene->GetWorld().GetView<ecs::CameraComponent>();
    view.ForEach([&](ecs::Entity entity, ecs::CameraComponent &camera) {
      camera.SetAspectRatio(width / static_cast<float>(height));
    });

    if (res != rhi::ERhiResult::Success) {
      avalon::Error("[Engine]: Failed to recreate swapchain!");
      return false;
    }
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
