module;
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <debug/assert.hpp>
#include <utility>

export module avalon.graphics:mesh_manager;

import :mesh;
import :types;
import :primitive_generator;
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

  MeshHandle GetDefaultMesh(EPrimitiveType primitiveType) {
    if (m_defaultMeshes.Contains(primitiveType)) {
      return *m_defaultMeshes.Get(primitiveType);
    }

    return CreateDefaultMesh(primitiveType);
  }

  MeshHandle CreateMesh(MeshData &&data) {
    if (data.positions.IsEmpty() || data.indices.IsEmpty()) {
      Error("[MeshManager]: Mesh data is invalid!");
      return {};
    }

    return m_meshPool.Create(std::move(data));
  }

  bool UploadMesh(MeshHandle handle, const VertexLayout &layout) {
    auto mesh = m_meshPool.Resolve(handle);
    if (!mesh) {
      Error("[MeshManager]: Invalid mesh handle! Handle: {}", handle.id);
      return false;
    }

    auto data = mesh->GetData();

    if (!IsValid(data, layout))
      return {};

    const EFormat iFormat = EFormat::R32_Uint;
    const size_t vertexCount = data.positions.GetSize();
    const size_t vStride = layout.stride;
    const size_t vBufferSize = vertexCount * vStride;
    const size_t iBufferSize = data.indices.GetSize() * GetFormatSize(iFormat);

    AVALON_ASSERT(sizeof(data.indices[0]) == GetFormatSize(iFormat));

    BufferCreateInfo vInfo{.size = vBufferSize,
                           .usage = EResourceUsage::VertexBuffer |
                                    EResourceUsage::TransferDst,
                           .memoryProperty = EMemoryProperty::DeviceLocal};

    auto vHandle = m_rhi.CreateBuffer(vInfo);

    BufferCreateInfo iInfo{.size = iBufferSize,
                           .usage = EResourceUsage::IndexBuffer |
                                    EResourceUsage::TransferDst,
                           .memoryProperty = EMemoryProperty::DeviceLocal};

    auto iHandle = m_rhi.CreateBuffer(iInfo);

    const size_t totalStagingSize = vBufferSize + iBufferSize;
    BufferHandle stagingHandle = m_rhi.CreateBuffer({
        .size = totalStagingSize,
        .usage = EResourceUsage::TransferSrc,
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
    mesh->Upload(vHandle, iHandle, iFormat);

    Debug("[MeshManager]: Mesh uploaded! Handle: {}, vbo size: {}", handle.id,
          vBufferSize);
    return true;
  }

  Mesh *Resolve(MeshHandle handle) { return m_meshPool.Resolve(handle); }

private:
  MeshHandle CreateDefaultMesh(EPrimitiveType type) {
    graphics::MeshData data;
    switch (type) {
    case EPrimitiveType::Cube:
      data = PrimitiveGenerator::GenerateCube();
      break;
    case EPrimitiveType::Plane:
      data = PrimitiveGenerator::GeneratePlane();
      break;
    case EPrimitiveType::Quad:
      data = PrimitiveGenerator::GenerateQuad();
      break;
    case EPrimitiveType::Sphere:
      data = PrimitiveGenerator::GenerateSphere();
      break;
    }

    auto meshHandle = CreateMesh(std::move(data));
    m_defaultMeshes.Insert(type, meshHandle);
    return meshHandle;
  }

  IRhi &m_rhi;
  mem::ResourcePool<Mesh> m_meshPool;

  HashMap<EPrimitiveType, MeshHandle> m_defaultMeshes;
};

} // namespace avalon::graphics
