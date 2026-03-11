module;
#include <expected>
#include <vulkan/vulkan.h>

export module avalon.rhi.vulkan;

import avalon.rhi;
import avalon.core;
import :types;
import :command_buffer;
import :pipeline_manager;
import :device_context;
import :swapchain_context;
import :resource_pool;

namespace avalon::rhi {

class VkRhi final : public IRhi, public IRenderResourceProvider {

public:
  VkRhi();
  ~VkRhi() override;
  auto OnLoad() -> EStatusCode override;
  auto Initialize(const DeviceRequirement &requirement,
                  const window::NativeWindowInfo &inWindowInfo, uint32_t width,
                  uint32_t height) -> ERhiResult override;

  auto CreatePipeline(const PipelineCreateInfo &desc)
      -> PipelineHandle override;
  auto GetSwapchainImageFormat() -> EFormat override;
  auto RecreateSwapchain(RenderPassHandle handle, uint32_t width,
                         uint32_t height) -> ERhiResult override;

  auto CreateBuffer(const BufferCreateInfo &info) -> BufferHandle override;
  void ReleaseBuffer(BufferHandle handle) override;
  auto CreateRenderPass(const RenderPassCreateInfo &info)
      -> RenderPassHandle override;
  auto CreateCommandBuffer() -> ICommandBuffer * override;

  void SetSwapchainRenderPass(RenderPassHandle) override;

  void Submit(ICommandBuffer *cmd) override;
  auto BeginFrame() -> ERhiResult override;
  auto EndFrame() -> ERhiResult override;

  auto GetRenderPass(RenderPassHandle) -> const RenderPassResource & override;
  auto GetFrameBuffer(ERenderTarget) -> const FrameBufferResource & override;
  auto GetPipeline(PipelineHandle) -> const PipelineResource & override;
  auto GetBuffer(BufferHandle) -> const BufferResource & override;

private:
  auto CreateCommandPools() -> std::expected<void, ERhiResult>;
  auto CreateSyncObjects() -> std::expected<void, ERhiResult>;
  void CleanupSwapchainFrameBuffers();
  void CreateSwapchianFrameBuffers(RenderPassHandle handle);

private:
  struct FrameSyncObject {
    VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphore{VK_NULL_HANDLE};
    VkFence m_inflightFence;
  };

  UniquePtr<DeviceContext> m_context;
  UniquePtr<SwapchainContext> m_swapchainContext;
  UniquePtr<ResourcePool> m_resourcePool;
  UniquePtr<PipelineManager> m_pipelineManager;

  Array<VkCommandPool> m_commandPools;
  Array<UniquePtr<CommandBuffer>> m_commandBuffers;
  Array<FrameSyncObject> m_frameSyncObjects;
  uint32_t m_currentImageIndex = 0;
  uint32_t m_currentFrame = 0;
  uint32_t m_maxFrameInFlight;
};
} // namespace avalon::rhi
