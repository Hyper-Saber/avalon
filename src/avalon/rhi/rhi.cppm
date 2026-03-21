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
  virtual auto Initialize(const DeviceRequirement &requriement,
                          const window::NativeWindowInfo &windowInfo = {},
                          uint32_t width = 0, uint32_t height = 0)
      -> ERhiResult = 0;

  virtual auto GetSwapchainImageFormat() const -> EFormat = 0;
  virtual auto GetMainCommandBuffer() const -> ICommandBuffer * = 0;
  virtual uint32_t GetCurrentFrameIndex() const = 0;
  virtual uint32_t GetMaxFrameInFlight() const = 0;
  virtual auto GetCapabilities() const -> DeviceCapabilities = 0;

  virtual auto RecreateSwapchain(RenderPassHandle handle, uint32_t width,
                                 uint32_t height) -> ERhiResult = 0;

  virtual auto CreateRenderPass(const RenderPassCreateInfo &info)
      -> RenderPassHandle = 0;
  virtual auto CreatePipeline(const PipelineCreateInfo &info)
      -> PipelineHandle = 0;

  virtual auto CreateBuffer(const BufferCreateInfo &info) -> BufferHandle = 0;
  virtual void ReleaseBuffer(BufferHandle handle) = 0;

  virtual auto CreateDescriptorWriter(PipelineHandle handle, uint32_t set)
      -> IDescriptorWriter & = 0;

  virtual void
  ExcuteOnce(EQueueType queueType,
             const std::function<void(ICommandBuffer *)> &action) = 0;

  virtual auto MapMemory(BufferHandle handle) -> void * = 0;
  virtual void UnmapMemory(BufferHandle handle) = 0;

  virtual void Submit(ICommandBuffer *cmd) = 0;
  virtual auto BeginFrame() -> ERhiResult = 0;
  virtual auto EndFrame() -> ERhiResult = 0;

  virtual void WaitIdle() = 0;
};
} // namespace avalon::rhi
