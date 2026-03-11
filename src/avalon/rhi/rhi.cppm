module;
#include <cstdint>

export module avalon.rhi;
export import :command_buffer;
export import :types;
export import :utils;
import avalon.core;

export namespace avalon::rhi {

class IRhi : public IPlugin {
public:
  virtual auto Initialize(const DeviceRequirement &requriement,
                          const window::NativeWindowInfo &windowInfo = {},
                          uint32_t width = 0, uint32_t height = 0)
      -> ERhiResult = 0;

  virtual auto GetSwapchainImageFormat() -> EFormat = 0;

  virtual auto RecreateSwapchain(RenderPassHandle handle, uint32_t width,
                                 uint32_t height) -> ERhiResult = 0;

  virtual auto CreatePipeline(const PipelineCreateInfo &info)
      -> PipelineHandle = 0;

  virtual auto CreateBuffer(const BufferCreateInfo &info) -> BufferHandle = 0;
  virtual void ReleaseBuffer(BufferHandle handle) = 0;
  virtual auto CreateCommandBuffer() -> ICommandBuffer * = 0;

  virtual auto CreateRenderPass(const RenderPassCreateInfo &info)
      -> RenderPassHandle = 0;
  virtual void SetSwapchainRenderPass(RenderPassHandle) = 0;

  virtual void Submit(ICommandBuffer *cmd) = 0;
  virtual auto BeginFrame() -> ERhiResult = 0;
  virtual auto EndFrame() -> ERhiResult = 0;
};
} // namespace avalon::rhi
