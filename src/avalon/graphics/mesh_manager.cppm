module;
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <debug/assert.hpp>

export module avalon.graphics:mesh_manager;

import :mesh;
import avalon.core;

using namespace avalon::rhi;

namespace {
using namespace avalon::graphics;
using namespace avalon;

bool IsValid(const MeshData &data, const VertexLayout &layout) noexcept {
  if (data.positions.IsEmpty() || data.indices.IsEmpty())
    return false;

  auto vertexCount = data.positions.GetSize();

  for (auto &attr : layout.attributes) {
    switch (attr.semantic) {
    case EVertexSemantic::Unknown:
      AVALON_ASSERT(false);
      return false;
    case EVertexSemantic::Position:
      break;
    case EVertexSemantic::TexCoord:
      if (data.texCoords.IsEmpty() || data.texCoords.GetSize() != vertexCount)
        return false;
      break;
    case EVertexSemantic::Color:
      if (data.colors.IsEmpty() || data.colors.GetSize() != vertexCount)
        return false;
      break;
    case EVertexSemantic::Normal:
      if (data.normals.IsEmpty() || data.normals.GetSize() != vertexCount)
        return false;
      break;
    }
  }

  return true;
}

void InterleaveVertexData(const MeshData &data, const VertexLayout &layout,
                          uint8_t *pOutMapped) {
  auto vertexCount = data.positions.GetSize();
  auto stride = layout.stride;

  for (size_t i = 0; i < vertexCount; i++) {
    auto pVertexStart = pOutMapped + (i * stride);

    for (const auto &attr : layout.attributes) {
      auto targetAddr = pVertexStart + attr.offset;
      switch (attr.semantic) {
      case EVertexSemantic::Position:
        std::memcpy(targetAddr, &data.positions[i], sizeof(Vec3));
        break;
      case EVertexSemantic::TexCoord:
        std::memcpy(targetAddr, &data.texCoords[i], sizeof(Vec2));
        break;
      case EVertexSemantic::Color:
        std::memcpy(targetAddr, &data.colors[i], sizeof(Vec3));
        break;
      case EVertexSemantic::Normal:
        std::memcpy(targetAddr, &data.normals[i], sizeof(Vec3));
        break;
      default:
        break;
      }
    }
  }
}

} // namespace

export namespace avalon::graphics {

class AVALON_GRAPHICS_API MeshManager final
    : public mem::AutoDestroyable<MeshManager> {
public:
  explicit MeshManager(IRhi &rhi) : m_rhi(rhi) {}

  MeshHandle CreateMesh(const MeshData &data, const VertexLayout &layout) {
    if (!IsValid(data, layout))
      return {};

    const EFormat iFormat = EFormat::R32_Uint;
    const size_t vertexCount = data.positions.GetSize();
    const size_t vStride = layout.stride;
    const size_t vBufferSize = vertexCount * vStride;
    const size_t iBufferSize = data.indices.GetSize() * GetFormatSize(iFormat);

    AVALON_ASSERT(sizeof(data.indices[0]) == GetFormatSize(iFormat));

    BufferCreateInfo vInfo{.size = vBufferSize,
                           .usage =
                               EBufferUsage::Vertex | EBufferUsage::TransferDst,
                           .memoryProperty = EMemoryProperty::DeviceLocal};

    auto vHandle = m_rhi.CreateBuffer(vInfo);

    BufferCreateInfo iInfo{.size = iBufferSize,
                           .usage =
                               EBufferUsage::Index | EBufferUsage::TransferDst,
                           .memoryProperty = EMemoryProperty::DeviceLocal};

    auto iHandle = m_rhi.CreateBuffer(iInfo);

    const size_t totalStagingSize = vBufferSize + iBufferSize;
    BufferHandle stagingHandle = m_rhi.CreateBuffer({
        .size = totalStagingSize,
        .usage = EBufferUsage::TransferSrc,
        .memoryProperty =
            EMemoryProperty::HostVisible | EMemoryProperty::HostCoherent,
    });

    auto pMapped = static_cast<uint8_t *>(m_rhi.MapMemory(stagingHandle));
    InterleaveVertexData(data, layout, pMapped);
    std::memcpy(pMapped + vBufferSize, data.indices.GetData(), iBufferSize);
    m_rhi.UnmapMemory(stagingHandle);

    m_rhi.ExcuteOnce(rhi::EQueueType::Transfer, [&](auto cmd) {
      BufferCopy vRegion{
          .srcOffset = 0,
          .dstOffset = 0,
          .size = vBufferSize,
      };
      BufferCopy iRegion{
          .srcOffset = vBufferSize,
          .dstOffset = 0,
          .size = iBufferSize,
      };
      cmd->CopyBuffer(stagingHandle, vHandle, vRegion);
      cmd->CopyBuffer(stagingHandle, iHandle, iRegion);
    });

    m_rhi.ReleaseBuffer(stagingHandle);

    return m_meshPool.Create(vHandle, iHandle, data.indices.GetSize(), iFormat);
  }

  Mesh *Resolve(MeshHandle handle) { return m_meshPool.Resolve(handle); }

private:
  IRhi &m_rhi;
  mem::ResourcePool<Mesh> m_meshPool;
};

} // namespace avalon::graphics
