module;
#include <debug/assert.hpp>
#include <optional>
module avalon.graphics;

import avalon.core;
import avalon.rhi;
import :render_graph_builder;
import :render_graph;

namespace {} // namespace

namespace avalon::graphics {

rhi::EFormat
RenderGraphBuilder::SpecifyTextureDefaultFormat(rhi::EResourceUsage usage) {
  switch (usage) {
  case avalon::rhi::EResourceUsage::None:
  case avalon::rhi::EResourceUsage::VertexBuffer:
  case avalon::rhi::EResourceUsage::IndexBuffer:
  case avalon::rhi::EResourceUsage::IndirectBuffer:
  case avalon::rhi::EResourceUsage::UniformBuffer:
  case avalon::rhi::EResourceUsage::StorageBuffer:
    AVALON_ASSERT_MSG(false,
                      String::Format("[RenderGraph] Unsupported usage {}, try "
                                     "SpecifyBufferDefaultFormat instead!",
                                     ToView(usage)));
    break;
  case avalon::rhi::EResourceUsage::ReadOnly:
  case avalon::rhi::EResourceUsage::ReadWrite:
  case avalon::rhi::EResourceUsage::ColorAttachment:
  case avalon::rhi::EResourceUsage::TransferSrc:
  case avalon::rhi::EResourceUsage::TransferDst:
    return rhi::EFormat::R16G16B16A16_SFLOAT;
  case avalon::rhi::EResourceUsage::DepthStencilAttachment:
    return rhi::EFormat::D32_SFLOAT_S8_UINT;
  case avalon::rhi::EResourceUsage::Present:
    return m_rhi->GetSwapchainImageFormat();
  }
  AVALON_ASSERT_MSG(
      false, String::Format(
                 "[RenderGraph] Failed To specify default format for usage {}!",
                 ToView(usage)));
  return rhi::EFormat::Undefined;
}

auto RenderGraphBuilder::Write(StringId name, rhi::EResourceUsage usage)
    -> VirtualResourceHandle {
  m_desc.nameHash = name;
  m_desc.usage = usage;

  if (m_desc.format == rhi::EFormat::Undefined) {
    m_desc.format = SpecifyTextureDefaultFormat(usage);
  }

  if (!m_desc.extent.IsValid()) {
    m_desc.extent = m_rhi->GetSwapchainExtent();
  }

  if (!m_clearValue) {
    if (IsDepthFormat(m_desc.format))
      m_clearValue = ClearValue::DepthStencil();
    else
      m_clearValue = ClearValue::Black();
  }

  m_currentResource = m_graph.Write(*m_owner, m_desc);

  auto &node = m_graph.GetNode(m_owner);
  for (auto &request : node.outputs) {
    if (request.handle == m_currentResource) {
      request.loadOp = rhi::EAttachmentLoadOp::Clear;
      request.storeOp = rhi::EAttachmentStoreOp::Store;
      request.clearValue = m_clearValue.value();
      break;
    }
  }

  m_clearValue = std::nullopt;
  m_desc = {};
  return m_currentResource;
}

auto RenderGraphBuilder::Read(StringId name, rhi::EResourceUsage usage)
    -> VirtualResourceHandle {
  return m_graph.Read(*m_owner, name, usage);
}

auto RenderGraphBuilder::SetExtent(const rhi::Extent2D &extent)
    -> RenderGraphBuilder & {
  m_desc.extent = extent;
  return *this;
}

auto RenderGraphBuilder::SetRelativeExtent(float scale)
    -> RenderGraphBuilder & {
  m_desc.extent = m_rhi->GetSwapchainExtent() * scale;
  return *this;
}

auto RenderGraphBuilder::SetFormat(rhi::EFormat format)
    -> RenderGraphBuilder & {
  m_desc.format = format;
  return *this;
}

auto RenderGraphBuilder::SetSamplerCount(rhi::ESampleCount count)
    -> RenderGraphBuilder & {
  m_desc.sampleCount = count;
  return *this;
}

auto RenderGraphBuilder::SetLoadOp(rhi::EAttachmentLoadOp op)
    -> RenderGraphBuilder & {
  auto &node = m_graph.GetNode(m_owner);
  for (auto &request : node.outputs) {
    if (request.handle == m_currentResource) {
      request.loadOp = op;
      break;
    }
  }
  return *this;
}

auto RenderGraphBuilder::SetStoreOp(rhi::EAttachmentStoreOp op)
    -> RenderGraphBuilder & {
  auto &node = m_graph.GetNode(m_owner);
  for (auto &request : node.outputs) {
    if (request.handle == m_currentResource) {
      request.storeOp = op;
      break;
    }
  }
  return *this;
}

auto RenderGraphBuilder::SetClearValue(rhi::ClearValue value)
    -> RenderGraphBuilder & {
  m_clearValue = value;
  return *this;
}

} // namespace avalon::graphics
