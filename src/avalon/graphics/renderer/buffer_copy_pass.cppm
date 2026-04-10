module;
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.graphics:buffer_copy_pass;

import avalon.core;
import avalon.rhi;
import :render_pass;
import :render_graph_builder;
import :render_context;
import :types;
import :utils;
import :renderer_types;

namespace avalon::graphics {

export class AVALON_GRAPHICS_API BufferCopyPass final
    : public RenderPass<BufferCopyPass> {
public:
  BufferCopyPass(StringId src, StringId dst, uint32_t size, uint32_t srcOffset,
                 uint32_t dstOffset)
      : m_src(src), m_dst(dst), m_size(size), m_srcOffset(srcOffset),
        m_dstOffset(dstOffset) {}

  void Setup(RenderGraphBuilder &builder) override {
    m_srcHandle = builder.ReadBuffer(m_src, EResourceUsage::TransferSrc,
                                     EShaderStage::None);
    m_dstHandle = builder.WriteBuffer(m_dst, EResourceUsage::UniformBuffer,
                                      EResourceUsage::TransferDst,
                                      sizeof(CubemapSH), EShaderStage::None);
  }

  void OnCompile(rhi::IRhi &rhi) override {}

  void Execute(rhi::ICommandBuffer &cmd, RenderContext &context) override {
    auto srcAllocation = context.GetPhysicalBufferAllocation(m_srcHandle);
    auto dstAllocation = context.GetPhysicalBufferAllocation(m_dstHandle);

    BufferCopyRegion region{
        .srcOffset = srcAllocation.offset + m_srcOffset,
        .dstOffset = dstAllocation.offset + m_dstOffset,

        .size = m_size,
    };

    cmd.CopyBuffer(srcAllocation.buffer, dstAllocation.buffer, region);
  }

private:
  StringId m_src;
  StringId m_dst;
  uint32_t m_srcOffset;
  uint32_t m_dstOffset;
  uint32_t m_size;
  VirtualResourceHandle m_srcHandle;
  VirtualResourceHandle m_dstHandle;
};

} // namespace avalon::graphics
