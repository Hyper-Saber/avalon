module;
#include <expected>
#include <functional>
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

  void SetSwapchainRenderPass(RenderPassHandle) override;
  auto GetSwapchainImageFormat() -> EFormat override;

  auto CreateRenderPass(const RenderPassCreateInfo &info)
      -> RenderPassHandle override;
  auto CreatePipeline(const PipelineCreateInfo &desc)
      -> PipelineHandle override;

  auto RecreateSwapchain(RenderPassHandle handle, uint32_t width,
                         uint32_t height) -> ERhiResult override;

  auto CreateBuffer(const BufferCreateInfo &info) -> BufferHandle override;
  void ReleaseBuffer(BufferHandle handle) override;

  auto CreateCommandBuffer() -> ICommandBuffer * override;

  void ExcuteOnce(EQueueType queueType,
                  const std::function<void(ICommandBuffer *)> &action) override;

  void *MapMemory(BufferHandle handle) override;
  void UnmapMemory(BufferHandle handle) override;

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

  UniquePtr<DeviceContext> m_deviceContext;
  UniquePtr<SwapchainContext> m_swapchainContext;
  UniquePtr<ResourcePool> m_resourcePool;
  UniquePtr<PipelineManager> m_pipelineManager;

  VkCommandPool m_immTransferPool;
  Array<VkCommandPool> m_frameCommandPools;
  Array<UniquePtr<CommandBuffer>> m_frameCommandBuffers;
  Array<FrameSyncObject> m_frameSyncObjects;
  uint32_t m_currentImageIndex = 0;
  uint32_t m_currentFrame = 0;
  uint32_t m_maxFrameInFlight;
};
} // namespace avalon::rhi
