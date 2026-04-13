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
                          uint8_t *pPosUVMapped, uint8_t *pAttrMapped) {
  auto vertexCount = data.positions.GetSize();

  for (size_t i = 0; i < vertexCount; i++) {
    uint8_t *pVertexStarts[2] = {pPosUVMapped + (i * layout.strides[0]),
                                 pAttrMapped + (i * layout.strides[1])};

    for (const auto &attr : layout.attributes) {
      auto targetAddr = pVertexStarts[attr.binding] + attr.offset;

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

  MeshHandle GetDefaultMesh(ESDFType primitiveType) {
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

  bool UploadStandardMesh(MeshHandle handle) {
    return UploadMesh(handle, VertexLayout::GetStandardLayout());
  }

  bool UploadMesh(MeshHandle handle, const VertexLayout &layout) {
    auto mesh = m_meshPool.Resolve(handle);
    if (!mesh) {
      Error("[MeshManager]: Invalid mesh handle! Handle: {}", handle.id);
      return false;
    }

    auto data = mesh->GetData();
    if (!IsValid(data, layout))
      return false;

    const size_t vertexCount = data.positions.GetSize();
    const size_t indexCount = data.indices.GetSize();
    const size_t iBufferSize = indexCount * sizeof(uint32_t);

    size_t posSize =
        layout.bindingUsed[0] ? vertexCount * layout.strides[0] : 0;
    size_t attrSize =
        layout.bindingUsed[1] ? vertexCount * layout.strides[1] : 0;

    rhi::BufferAllocation posAlloc =
        (posSize > 0) ? m_rhi.AllocateVertexGeometrySSBO(posSize)
                      : rhi::BufferAllocation{};

    rhi::BufferAllocation attrAlloc =
        (attrSize > 0) ? m_rhi.AllocateVertexAttributesSSBO(attrSize)
                       : rhi::BufferAllocation{};

    rhi::BufferAllocation indexAlloc =
        m_rhi.AllocateVertexIndicesSSBO(iBufferSize);

    InterleaveVertexData(data, layout,
                         static_cast<uint8_t *>(posAlloc.pHostAddress),
                         static_cast<uint8_t *>(attrAlloc.pHostAddress));

    std::memcpy(indexAlloc.pHostAddress, data.indices.GetData(), iBufferSize);

    mesh->SetGPUPointers(posAlloc.buffer, posAlloc.offset, attrAlloc.buffer,
                         attrAlloc.offset, indexAlloc.buffer, indexAlloc.offset,
                         static_cast<uint32_t>(vertexCount),
                         static_cast<uint32_t>(indexCount));

    Debug("[MeshManager]: Mesh allocated in Global SSBO! Handle: {}, "
          "PosOffset: {}, AttrOffset: {}, IndexOffset: {}",
          handle.id, posAlloc.offset, attrAlloc.offset, indexAlloc.offset);

    return true;
  }

  Mesh *Resolve(MeshHandle handle) { return m_meshPool.Resolve(handle); }

private:
  MeshHandle CreateDefaultMesh(ESDFType type) {
    graphics::MeshData data;
    switch (type) {
    case ESDFType::Cube:
      data = PrimitiveGenerator::GenerateCube();
      break;
    case ESDFType::Plane:
      data = PrimitiveGenerator::GeneratePlane();
      break;
    case ESDFType::Quad:
      data = PrimitiveGenerator::GenerateQuad();
      break;
    case ESDFType::Sphere:
      data = PrimitiveGenerator::GenerateSphere();
      break;
    default:
      Error("No default mesh found !");
      break;
    }

    auto meshHandle = CreateMesh(std::move(data));
    m_defaultMeshes.Insert(type, meshHandle);
    return meshHandle;
  }

  IRhi &m_rhi;
  mem::ResourcePool<Mesh> m_meshPool;

  HashMap<ESDFType, MeshHandle> m_defaultMeshes;
};

} // namespace avalon::graphics
