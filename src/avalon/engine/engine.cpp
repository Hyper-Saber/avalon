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

constexpr StringView kShaderFolderPath = "/tests/shaders/";
constexpr StringView kShaderFolderVirtualPath = "shader:";

auto Engine::Initialize(const EngineConfig &config)
    -> std::expected<void, EStatusCode> {
  m_config = config;

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

  m_renderer = MakeUnique<graphics::Renderer>(*m_rhi.Get());

  rhi::Extent2D extent = {width, height};

  rhi::AttachmentDescription colorAttachment{
      .format = m_rhi->GetSwapchainImageFormat(),
      .loadOp = rhi::EAttachmentLoadOp::Clear,
      .storeOp = rhi::EAttachmentStoreOp::Store,
      .initialLayout = rhi::EResourceLayout::Undefined,
      .finalLayout = rhi::EResourceLayout::Present,
  };

  rhi::AttachmentDescription depthAttachment{
      .format = rhi::EFormat::D32_SFLOAT_S8_UINT,
      .loadOp = rhi::EAttachmentLoadOp::Clear,
      .storeOp = rhi::EAttachmentStoreOp::DontCare,
      .initialLayout = rhi::EResourceLayout::Undefined,
      .finalLayout = rhi::EResourceLayout::DepthStencilAttachment,
  };

  rhi::RenderPassCreateInfo passCreateInfo{
      .colorAttachments = {colorAttachment},
      .depthAttachment = depthAttachment,
      .hasDepth = true,
  };

  m_renderPass = m_rhi->CreateRenderPass(passCreateInfo);
  m_rhi->SetSwapchainRenderPass(m_renderPass);

  auto shaderHandle = graphics::GetShaderManager().GetOrCreateShader(
      shaderVirtualFolderPath / StringView("test.hlsl"));
  auto material = graphics::Material(shaderHandle);

  auto pipelineCreateInfo = material.GetPipelineCreateInfo();
  pipelineCreateInfo.renderPassHandle = m_renderPass;

  m_pipeline = m_rhi->CreatePipeline(pipelineCreateInfo);

  m_renderer->AddPass(
      MakeUnique<graphics::OpaquePass>(m_pipeline, m_renderPass, extent));

  m_world = MakeUnique<ecs::World>();

  m_model = CreateGeometryEntity(material);

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
  m_renderer->Render(*cmd, *m_world.Get());
  cmd->End();
  m_rhi->Submit(cmd);
  return beginRes != rhi::ERhiResult::Success ? beginRes : m_rhi->EndFrame();
}

void Engine::Update() {
  static float totalTime = 0.f;
  totalTime += m_deltaTime;

  auto transform = m_world->GetComponent<graphics::TransformComponent>(m_model);
  transform->local.rotation.z = totalTime * 90;
  transform->local.rotation.x = totalTime * 60;
  transform->local.rotation.y = totalTime * 30;
  transform->local.position.x = std::sin(totalTime) * 0.5f;
  transform->local.position.y = std::cos(totalTime) * 0.5f;
  transform->local.scale = Vec3::One() * std::sin(totalTime) * 0.4f + 0.8f;
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

ecs::Entity Engine::CreateGeometryEntity(const graphics::Material &material) {
  graphics::MeshData data{.positions{// Front face (Z = 0.5)
                                     {-0.5f, -0.5f, 0.5f},
                                     {0.5f, -0.5f, 0.5f},
                                     {0.5f, 0.5f, 0.5f},
                                     {-0.5f, 0.5f, 0.5f},
                                     // Back face (Z = -0.5)
                                     {-0.5f, -0.5f, -0.5f},
                                     {0.5f, -0.5f, -0.5f},
                                     {0.5f, 0.5f, -0.5f},
                                     {-0.5f, 0.5f, -0.5f}},
                          .indices{// Front
                                   0, 1, 2, 2, 3, 0,
                                   // Right
                                   1, 5, 6, 6, 2, 1,
                                   // Back
                                   7, 6, 5, 5, 4, 7,
                                   // Left
                                   4, 0, 3, 3, 7, 4,
                                   // Top
                                   3, 2, 6, 6, 7, 3,
                                   // Bottom
                                   4, 5, 1, 1, 0, 4},
                          .colors{
                              {1, 0, 0, 1},
                              {0, 1, 0, 1},
                              {0, 0, 1, 1},
                              {1, 1, 0, 1}, // 前四个顶点颜色
                              {1, 0, 1, 1},
                              {0, 1, 1, 1},
                              {1, 1, 1, 1},
                              {0, 0, 0, 1} // 后四个顶点颜色
                          },
                          .texCoords{{0.f, 0.f},
                                     {1.f, 0.f},
                                     {1.f, 1.f},
                                     {0.f, 1.f},
                                     {0.f, 0.f},
                                     {1.f, 0.f},
                                     {1.f, 1.f},
                                     {0.f, 1.f}}};

  m_mesh =
      graphics::GetMeshManager().CreateMesh(data, material.GetVertexLayout());

  auto entity = m_world->CreateEntity();
  Transform transform;
  m_world->AddComponent<graphics::MeshComponent>(entity, m_mesh)
      ->AddComponent<graphics::TransformComponent>(entity, transform);

  return entity;
}

} // namespace avalon
