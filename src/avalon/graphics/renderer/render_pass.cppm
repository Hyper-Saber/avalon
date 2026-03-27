module;
#include <concepts>
export module avalon.graphics:render_pass;

import avalon.core;
import avalon.rhi;
import :renderer_types;
import :render_context;

export namespace avalon::graphics {

class IRenderPass : public NonCopyable, public mem::IAutoDestroyable {
public:
  virtual ~IRenderPass() = default;

  virtual void Setup(class RenderGraphBuilder &builder) = 0;
  virtual void OnCompile(rhi::IRhi &rhi) = 0;

  virtual void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) = 0;
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

template <typename T>
concept TRenderPass = std::derived_from<T, IRenderPass>;

} // namespace avalon::graphics
