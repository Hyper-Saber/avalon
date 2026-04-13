module;
#include <cstdint>
#include <functional>

export module avalon.rhi:rhi;
import :command_buffer;
import :types;
import :descriptor_writer;

import avalon.core;

export namespace avalon::rhi {

class IRhi : public IPlugin {
public:
  inline static DeviceCapabilities capabilities;

  virtual auto Initialize(const DeviceRequirement &requriement,
                          const window::NativeWindowInfo &windowInfo = {},
                          uint32_t width = 0, uint32_t height = 0)
      -> ERhiResult = 0;

  virtual auto GetUBOPool() const -> class RingBufferPool & = 0;
  virtual auto GetDynamicSSBOPool() const -> RingBufferPool & = 0;
  virtual auto GetBindlessManager() const -> class IBindlessManager & = 0;

  virtual void UpdateMaterialBuffer(size_t offset, const void *data,
                                    size_t size) = 0;

  virtual auto AllocateIndirectSSBO(size_t size) -> BufferAllocation = 0;
  virtual auto AllocateStaticSSBO(size_t size) -> BufferAllocation = 0;
  virtual auto AllocateVertexGeometrySSBO(size_t size) -> BufferAllocation = 0;
  virtual auto AllocateVertexAttributesSSBO(size_t size)
      -> BufferAllocation = 0;
  virtual auto AllocateVertexIndicesSSBO(size_t size) -> BufferAllocation = 0;

  virtual auto GetIndexBuffer() const -> BufferHandle = 0;

  virtual auto GetStaticSamplers() const -> const StaticSamplers & = 0;
  virtual auto GetSwapchainImageFormat() const -> EFormat = 0;
  virtual auto GetSwapchainExtent() const -> Extent2D = 0;
  virtual auto GetCurrentPresentTexture() -> TextureHandle = 0;
  virtual auto GetMainCommandBuffer() const -> ICommandBuffer * = 0;
  virtual uint32_t GetCurrentFrameIndex() const = 0;
  virtual uint32_t GetMaxFrameInFlight() const = 0;
  virtual auto GetDefaultTexture() const -> TextureHandle = 0;
  virtual auto GetCapabilities() const -> DeviceCapabilities = 0;
  virtual auto GetTextureCreateInfo(TextureHandle) const
      -> TextureCreateInfo = 0;

  virtual auto RecreateSwapchain(uint32_t width, uint32_t height)
      -> ERhiResult = 0;

  virtual auto GetOrCreatePipeline(const PipelineCreateInfo &info)
      -> PipelineHandle = 0;

  virtual auto GetOrCreateComputePipeline(const ComputePipelineCreateInfo &info)
      -> PipelineHandle = 0;

  virtual auto GetDummyComputePipeline() const -> PipelineHandle = 0;

  virtual auto CreateBuffer(const BufferCreateInfo &info) -> BufferHandle = 0;
  virtual void ReleaseBuffer(BufferHandle handle) = 0;

  virtual auto CreateTexture(const TextureCreateInfo &info)
      -> TextureHandle = 0;
  virtual void ReleaseTexture(TextureHandle handle) = 0;

  virtual auto GetSceneGlobalSetWriter() -> IDescriptorWriter & = 0;
  virtual auto CreateDescriptorWriter(PipelineHandle handle, uint32_t set)
      -> IDescriptorWriter & = 0;

  virtual void
  ExcuteOnce(EQueueType queueType,
             const std::function<void(ICommandBuffer *)> &action) = 0;

  virtual auto MapMemory(BufferHandle handle) -> void * = 0;
  virtual void UnmapMemory(BufferHandle handle) = 0;

  virtual void Submit(ICommandBuffer &cmd) = 0;
  virtual auto BeginFrame() -> ERhiResult = 0;
  virtual auto EndFrame() -> ERhiResult = 0;

  virtual void WaitIdle() = 0;
};
} // namespace avalon::rhi
