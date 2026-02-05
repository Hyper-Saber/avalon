module;
#include <bits/stdint-uintn.h>
#include <expected>

export module avalon.rhi;
import avalon.core;

export namespace avalon::rhi {

enum class ERhiError {
  Unknown,
  InitializationFailed,
  SurfaceLost,
  DeviceLost,
  OutOfMemory,
  BackendSpecificError,
  SwapchainOutOfDate,
  FailedToRecordCommand,
  FailedToSubmitQueue,
};

class IRhi : public IPlugin {
public:
  virtual ~IRhi() = default;

  virtual auto Initialize(const window::NativeWindowInfo &windowInfo,
                          uint32_t width, uint32_t height)
      -> std::expected<void, EStatusCode> = 0;
  virtual auto RecreateSwapchain(uint32_t width, uint32_t height)
      -> std::expected<void, EStatusCode> = 0;
  virtual auto BeginFrame() -> std::expected<void, ERhiError> = 0;
  virtual auto EndFrame() -> std::expected<void, ERhiError> = 0;
};
} // namespace avalon::rhi
