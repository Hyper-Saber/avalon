module;
export module avalon.graphics:context;
import avalon.core;
import avalon.rhi;
import :types;

export namespace avalon::graphics {
struct GraphicsContext {
  static auto Get() -> GraphicsContext & {
    static GraphicsContext context;
    return context;
  }

  void Initialize(rhi::IRhi &rhi) {
    deviceCapabilities = rhi.GetCapabilities();
  }

  rhi::DeviceCapabilities deviceCapabilities;
};
} // namespace avalon::graphics
