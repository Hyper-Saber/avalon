export module avalon.graphics:render_pipeline;

import avalon.core;
import :render_graph_builder;
import :renderer_types;

export namespace avalon::graphics {

class IRenderPipeline : public virtual IRefCounted {
public:
  virtual ~IRenderPipeline() = default;

  virtual void Setup(RenderGraphBuilder &builder,
                     const RenderPacket &packet) = 0;

  virtual StringId GetName() const = 0;
};

template <typename T>
class RenderPipelineBase : public RefCounted<T>, public IRenderPipeline {
  // public:
  // void Destroy() override {
  //   T *pDerived = static_cast<T *>(this);
  //   pDerived->~T();
  //   mem::Allocator<T> alloc;
  //   alloc.Deallocate(pDerived, 1);
  // }
};

} // namespace avalon::graphics
