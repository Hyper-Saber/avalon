module;
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

  auto rhiLoadRes =
      LoadPlugin<rhi::IRhi>({kVkRhiPluginPath + kPluginExtension});

  if (!rhiLoadRes) {
    return std::unexpected(EStatusCode::PluginInitializeError);
  }
  m_rhi = std::move(rhiLoadRes.value());

  uint32_t width = config.windowProps.width, height = config.windowProps.height;
  rhi::ERhiResult result;
  if (config.renderDeviceRequirement.queueRequirement.isRequirePresent) {
    auto windowLoadRes =
        LoadPlugin<window::IWindow>({kWindowPluginPath + kPluginExtension});

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
      .format = rhi::EFormat::D32_SFLOAT,
      .loadOp = rhi::EAttachmentLoadOp::Clear,
      .storeOp = rhi::EAttachmentStoreOp::DontCare,
      .initialLayout = rhi::EResourceLayout::Undefined,
      .finalLayout = rhi::EResourceLayout::DepthStencilAttachment,
  };

  rhi::RenderPassCreateInfo passCreateInfo{
      .colorAttachments = {colorAttachment},
      .depthAttachment = depthAttachment,
      .hasDepth = false,
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

  return {};
}

void Engine::Run() {
  AVALON_ASSERT(m_window.Get() && m_rhi.Get());
  bool isRequestExit = false;
  while (!m_window->ShouldClose() && !isRequestExit) {
    m_window->PollEvents();

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
  auto beginRes = m_rhi->BeginFrame();
  if (beginRes != rhi::ERhiResult::Success) {
    TryHandleRhiError(beginRes);
    return beginRes;
  }
  auto cmd = m_rhi->CreateCommandBuffer();
  cmd->Begin();
  m_renderer->Render(*cmd, *m_world.Get());
  cmd->End();
  m_rhi->Submit(cmd);
  return beginRes != rhi::ERhiResult::Success ? beginRes : m_rhi->EndFrame();
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

} // namespace avalon
