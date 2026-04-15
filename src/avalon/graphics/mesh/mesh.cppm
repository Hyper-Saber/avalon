module;
#include <cstdint>
#include <debug/assert.hpp>
#include <utility>
export module avalon.graphics:mesh;

import avalon.core;
import avalon.rhi;
import :types;

using namespace avalon::rhi;

export namespace avalon::graphics {

struct VertexLayout {
  bool bindingUsed[2]{false, false};
  uint32_t strides[2]{0, 0};
  Span<const VertexInputAttribute> attributes;

  static VertexLayout GetStandardLayout() {
    static Array<VertexInputAttribute> attributes = {
        {
            .location = 0,
            .binding = 0,
            .format = EFormat::R32G32B32_Float,
            .semantic = EVertexSemantic::Position,
            .offset = 0,
        },
        {
            .location = 1,
            .binding = 0,
            .format = EFormat::R32G32B32_Float,
            .semantic = EVertexSemantic::Normal,
            .offset = 12,
        },
        {
            .location = 2,
            .binding = 0,
            .format = EFormat::R32G32_Float,
            .semantic = EVertexSemantic::TexCoord,
            .offset = 24,
        },

        {
            .location = 3,
            .binding = 1,
            .format = EFormat::R32G32B32_Float,
            .semantic = EVertexSemantic::Color,
            .offset = 32,
        },
    };

    VertexLayout layout;
    layout.bindingUsed[0] = true;
    layout.bindingUsed[1] = true;
    layout.strides[0] = 32;
    layout.strides[1] = 12;
    layout.attributes = attributes;
    return layout;
  }
};

struct MeshData {
  struct SubMesh {
    StringId nameHash;
    uint32_t baseVertex;
    uint32_t startIndex;
    uint32_t indexCount;
  };

  Array<Vec3> positions;
  Array<uint32_t> indices;
  Array<Vec3> colors;
  Array<Vec3> normals;
  Array<Vec2> texCoords;
  Array<SubMesh> subMeshs;

  static AABB ComputeFullAABB(const MeshData &data) {
    AABB aabb;
    for (const auto &pos : data.positions) {
      aabb.Encapsulate(pos);
    }
    return aabb;
  }

  static AABB ComputeSubMeshAABB(const MeshData &data, uint32_t subMeshIndex) {
    if (subMeshIndex >= data.subMeshs.GetSize())
      return {};

    const auto &subMesh = data.subMeshs[subMeshIndex];
    AABB aabb;

    for (uint32_t i = 0; i < subMesh.indexCount; ++i) {
      uint32_t index = data.indices[subMesh.startIndex + i];
      const Vec3 &pos = data.positions[subMesh.baseVertex + index];
      aabb.Encapsulate(pos);
    }
    return aabb;
  }
};

class AVALON_GRAPHICS_API Mesh final : public mem::AutoDestroyable<Mesh> {
public:
  Mesh(MeshData &&data) : m_data(std::move(data)), m_isUploaded(false) {}

  void SetGPUPointers(BufferHandle geometryBuf, uint32_t geometryOffset,
                      BufferHandle attrBuf, uint32_t attrOffset,
                      BufferHandle indexBuf, uint32_t indexOffset,
                      uint32_t vertexCount, uint32_t indexCount) {
    m_geometryBuffer = geometryBuf;
    m_geometryOffset = geometryOffset;

    m_attrBuffer = attrBuf;
    m_attrOffset = attrOffset;

    m_indexBuffer = indexBuf;
    m_indexOffset = indexOffset;

    m_vertexCount = vertexCount;
    m_indexCount = indexCount;

    m_isUploaded = true;
  }

  void SetSDFType(ESDFType type) { m_sdfType = type; }
  void SetSDFSize(Vec3 size) { m_sdfSize = size; }
  void SetSDFTexture(TextureHandle handle) { m_sdfTexture = handle; }

  auto GetData() const noexcept -> const MeshData & { return m_data; }
  bool IsUploaded() const noexcept { return m_isUploaded; }

  BufferHandle GetGeometryBuffer() const noexcept { return m_geometryBuffer; }
  BufferHandle GetAttrBuffer() const noexcept { return m_attrBuffer; }
  BufferHandle GetIndexBuffer() const noexcept { return m_indexBuffer; }

  uint32_t GetGeometryOffset() const noexcept { return m_geometryOffset; }
  uint32_t GetAttributeOffset() const noexcept { return m_attrOffset; }
  uint32_t GetIndexOffset() const noexcept { return m_indexOffset; }

  uint32_t GetVertexCount() const noexcept { return m_vertexCount; }
  uint32_t GetIndexCount() const noexcept { return m_indexCount; }

  ESDFType GetSDFType() const noexcept { return m_sdfType; }
  Vec3 GetSDFExtent() const noexcept { return m_sdfSize; }
  TextureHandle GetSDFTexture() const noexcept { return m_sdfTexture; }

  EFormat GetIndexFormat() const noexcept { return EFormat::R32_Uint; }

private:
  MeshData m_data;
  bool m_isUploaded = false;

  BufferHandle m_geometryBuffer;
  uint32_t m_geometryOffset = 0;

  BufferHandle m_attrBuffer;
  uint32_t m_attrOffset = 0;

  BufferHandle m_indexBuffer;
  uint32_t m_indexOffset = 0;

  uint32_t m_vertexCount = 0;
  uint32_t m_indexCount = 0;

  ESDFType m_sdfType = ESDFType::Sphere;
  Vec3 m_sdfSize;
  TextureHandle m_sdfTexture;
};

} // namespace avalon::graphics
