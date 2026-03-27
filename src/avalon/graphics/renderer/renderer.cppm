module;
#include <cstdint>
export module avalon.graphics:renderer;

import avalon.core;
import :render_pipeline;

export namespace avalon::graphics {

class IRenderer : public NonCopyable, public mem::IAutoDestroyable {
public:
  virtual ~IRenderer() = default;

  virtual void OnResize(uint32_t width, uint32_t height) = 0;

  virtual void Render(const SceneSnapshot &) = 0;

  virtual void SetPipeline(IRenderPipeline *pipeline) = 0;

  virtual auto GetRhi() -> class rhi::IRhi & = 0;
};

auto AVALON_GRAPHICS_API CreateRenderer(class rhi::IRhi &rhi)
    -> UniquePtr<IRenderer>;

} // namespace avalon::graphics
