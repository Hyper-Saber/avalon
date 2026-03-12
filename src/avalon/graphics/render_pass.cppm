module;
export module avalon.graphics:render_pass;

import avalon.core;
import avalon.rhi;
import :mesh_extractor;

export namespace avalon::graphics {

struct RenderContext {
  rhi::ICommandBuffer &cmd;
};

class IRenderPass : public NonCopyable, public mem::IAutoDestroyable {
public:
  virtual ~IRenderPass() = default;
  virtual void SetClearColor(Color) = 0;
  virtual void OnResize(const rhi::Extent2D &extent) = 0;
  virtual void Setup(rhi::RenderPassBeginInfo &info) = 0;
  virtual void Execute(RenderContext &context, const RenderPacket &packet) = 0;
  virtual StringView GetName() const = 0;
};

template <typename T> class RenderPass : public IRenderPass {
public:
  void Destroy() override {
    T *pDerived = static_cast<T *>(this);
    pDerived->~T();
    mem::Allocator<T> alloc;
    alloc.Deallocate(pDerived, 1);
  }
};

} // namespace avalon::graphics
