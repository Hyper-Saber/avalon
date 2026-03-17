module;
#include <cstdint>
#include <debug/assert.hpp>
#include <utility>
export module avalon.graphics:mesh;

import avalon.core;
import avalon.rhi;

using namespace avalon::rhi;

export namespace avalon::graphics {

struct VertexLayout {
  uint32_t stride;
  Span<const VertexInputAttribute> attributes;
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

  struct AABB {
    float min[3];
    float max[3];
  } bounds;
};

class AVALON_GRAPHICS_API Mesh final : public mem::AutoDestroyable<Mesh> {
public:
  void Upload(BufferHandle vbo, BufferHandle ibo, EFormat indexFormat) {
    m_vertexBuffer = vbo;
    m_indexBuffer = ibo;
    m_indexFormat = indexFormat;
    m_isUploaded = true;
  }

  Mesh(MeshData &&data) : m_data(std::move(data)), m_isUploaded(false), m_indexCount(m_data.indices.GetSize()) {}

  auto GetData() const noexcept -> const MeshData & { return m_data; }

  BufferHandle GetVBO() const noexcept {
    AVALON_ASSERT(m_isUploaded);
    return m_vertexBuffer;
  }

  BufferHandle GetIBO() const noexcept {
    AVALON_ASSERT(m_isUploaded);
    return m_indexBuffer;
  }

  uint32_t GetIndexCount() const noexcept {
    AVALON_ASSERT(m_isUploaded);
    return m_indexCount;
  }

  EFormat GetIndexFormat() const noexcept {
    AVALON_ASSERT(m_isUploaded);
    return m_indexFormat;
  }

private:
  MeshData m_data;
  bool m_isUploaded;
  BufferHandle m_vertexBuffer;
  BufferHandle m_indexBuffer;
  uint32_t m_indexCount;
  EFormat m_indexFormat = EFormat::R32_Uint;
};

using MeshHandle = Handle<Mesh>;

} // namespace avalon::graphics
