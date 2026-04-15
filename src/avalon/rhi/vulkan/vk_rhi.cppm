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
import :descriptor_writer;
import :bindless_manager;

namespace avalon::rhi {

class VkRhi final : public IRhi, public IRenderResourceProvider {

public:
  VkRhi();
  ~VkRhi() override;
  auto OnLoad() -> EStatusCode override;
  auto Initialize(const DeviceRequirement &requirement,
                  const window::NativeWindowInfo &inWindowInfo, uint32_t width,
                  uint32_t height) -> ERhiResult override;

  auto GetUBOPool() const -> RingBufferPool & override;
  auto GetDynamicSSBOPool() const -> RingBufferPool & override;
  auto GetBindlessManager() const -> IBindlessManager & override;

  void UpdateMaterialBuffer(size_t offset, const void *data,
                            size_t size) override;

  auto AllocateIndirectSSBO(size_t size) -> BufferAllocation override;
  auto AllocateStaticSSBO(size_t size) -> BufferAllocation override;
  auto AllocateVertexGeometrySSBO(size_t size) -> BufferAllocation override;
  auto AllocateVertexAttributesSSBO(size_t size) -> BufferAllocation override;
  auto AllocateVertexIndicesSSBO(size_t size) -> BufferAllocation override;

  auto GetIndexBuffer() const -> BufferHandle override;

  auto GetStaticSamplers() const -> const StaticSamplers & override;
  auto GetMainCommandBuffer() const -> ICommandBuffer * override;
  auto GetSwapchainImageFormat() const -> EFormat override;
  auto GetCurrentPresentTexture() -> TextureHandle override;
  auto GetSwapchainExtent() const -> Extent2D override;
  uint32_t GetCurrentFrameIndex() const override;
  uint32_t GetMaxFrameInFlight() const override;
  auto GetCapabilities() const -> DeviceCapabilities override;
  auto GetDefaultTexture() const -> TextureHandle override;

  auto GetTextureCreateInfo(TextureHandle) const -> TextureCreateInfo override;
  auto GetOrCreatePipeline(const PipelineCreateInfo &desc)
      -> PipelineHandle override;
  auto GetOrCreateComputePipeline(const ComputePipelineCreateInfo &info)
      -> PipelineHandle override;

  auto GetDummyComputePipeline() const -> PipelineHandle override;

  auto RecreateSwapchain(uint32_t width, uint32_t height)
      -> ERhiResult override;

  auto CreateBuffer(const BufferCreateInfo &info) -> BufferHandle override;
  void ReleaseBuffer(BufferHandle handle) override;

  auto CreateTexture(const TextureCreateInfo &info) -> TextureHandle override;
  void ReleaseTexture(TextureHandle handle) override;

  auto GetSceneGlobalSetWriter() -> IDescriptorWriter & override;
  auto CreateDescriptorWriter(PipelineHandle handle, uint32_t set)
      -> IDescriptorWriter & override;

  void ExcuteOnce(EQueueType queueType,
                  const std::function<void(ICommandBuffer *)> &action) override;

  void *MapMemory(BufferHandle handle) override;
  void UnmapMemory(BufferHandle handle) override;

  void Submit(ICommandBuffer &cmd) override;
  auto BeginFrame() -> ERhiResult override;
  auto EndFrame() -> ERhiResult override;

  auto GetPipeline(PipelineHandle) -> const PipelineResource * override;
  auto GetBuffer(BufferHandle) -> const BufferResource * override;
  auto GetTexture(TextureHandle) -> const TextureResource * override;
  auto GetOrCreateMipStorageView(TextureHandle, uint32_t mipLevel)
      -> VkImageView override;
  auto GetOrCreateDepthTextureView(TextureHandle) -> VkImageView override;
  auto GetSampler(SamplerHandle) -> const SamplerResource * override;
  auto GetDescriptorSet(DescriptorSetHandle handle)
      -> const DescriptorSetResource * override;
  auto GetCurrentSwapchainImage() const -> VkImage override;
  auto GetBindlessSet() const -> VkDescriptorSet override;
  auto GetBindlessSetLayout() const -> VkDescriptorSetLayout override;
  auto GetSceneGlobalSet() const -> VkDescriptorSet override;
  auto GetSceneGlobalSetHandle() const -> DescriptorSetHandle override;
  auto GetSceneGlobalSetLayout() const -> VkDescriptorSetLayout override;
  uint32_t GetCurrentFrameIndex() override;
  uint32_t GetLastCompletedFrameIndex() override;
  auto GetMaterialSSBOInfo() const -> const VkDescriptorBufferInfo & override;
  auto GetIndirectSSBOInfo() const -> const VkDescriptorBufferInfo & override;
  auto GetDynamicSSBOInfo() const -> const VkDescriptorBufferInfo & override;
  auto GetStaticSSBOInfo() const -> const VkDescriptorBufferInfo & override;
  auto GetGeometriesSSBOInfo() const -> const VkDescriptorBufferInfo & override;
  auto GetAttributesSSBOInfo() const -> const VkDescriptorBufferInfo & override;
  auto GetIndicesSSBOInfo() const -> const VkDescriptorBufferInfo & override;

  void WaitIdle() override;

private:
  void CreateUBOPool();
  void CreateSSBOPool();
  auto CreateCommandPools() -> std::expected<void, ERhiResult>;
  auto CreateSyncObjects() -> std::expected<void, ERhiResult>;

  void CreateStaticSamplers();
  void CreateCommandBuffer();
  void WarpSwapchainTextures();

  void CreateStaticSSBOPool(UniquePtr<LinearBufferPool> &outUP,
                            VkDescriptorBufferInfo &outInfo, size_t size,
                            EResourceUsage usage);

  void CreateDynamicSSBOPool(UniquePtr<RingBufferPool> &outUP,
                             VkDescriptorBufferInfo &outInfo, size_t size,
                             EResourceUsage usage);

private:
  struct FrameSyncObject {
    VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphore{VK_NULL_HANDLE};
    VkFence m_inflightFence;
  };

  Array<TextureHandle> m_swapchainTextures;

  StaticSamplers m_staticSamplers;

  UniquePtr<RingBufferPool> m_uboPool;
  UniquePtr<RingBufferPool> m_indirectPool;
  UniquePtr<RingBufferPool> m_dynamicPool;

  UniquePtr<LinearBufferPool> m_materialPool;
  UniquePtr<LinearBufferPool> m_staticPool;
  UniquePtr<LinearBufferPool> m_geometryPool;
  UniquePtr<LinearBufferPool> m_attributesPool;
  UniquePtr<LinearBufferPool> m_indicesPool;

  VkDescriptorBufferInfo m_dynamicSSBODescriptorInfo;
  VkDescriptorBufferInfo m_indirectSSBODescriptorInfo;
  VkDescriptorBufferInfo m_materialSSBODescriptorInfo;
  VkDescriptorBufferInfo m_staticSSBODescriptorInfo;
  VkDescriptorBufferInfo m_geometrySSBOescriptorInfo;
  VkDescriptorBufferInfo m_attributesSSBODescriptorInfo;
  VkDescriptorBufferInfo m_indicesSSBODescriptorInfo;

  UniquePtr<DescriptorProvider> m_descriptorProvider;
  UniquePtr<BindlessManager> m_bindlessManager;
  UniquePtr<DeviceContext> m_deviceContext;
  UniquePtr<SwapchainContext> m_swapchainContext;
  UniquePtr<ResourcePool> m_resourcePool;
  UniquePtr<PipelineManager> m_pipelineManager;
  UniquePtr<StateTracker> m_stateTracker;
  UniquePtr<DescriptorWriter> m_descriptorWriter;
  UniquePtr<DescriptorWriter> m_sceneGlobalSetWriter;

  VkCommandPool m_immTransferPool;
  Array<VkCommandPool> m_frameCommandPools;
  Array<UniquePtr<CommandBuffer>> m_frameCommandBuffers;
  Array<FrameSyncObject> m_frameSyncObjects;
  uint32_t m_currentImageIndex = 0;
  uint32_t m_currentFrame = 0;
  uint32_t m_maxFrameInFlight;

  PipelineHandle m_dummyComputePipeline;

  TextureHandle m_defaultTexture;

  //
  //
  //
  //----------------------------debug------------------------------

#ifndef NDEBUG

private:
  void DumpMaterial(uint32_t index);
};

#endif // !NDEBUG
} // namespace avalon::rhi
