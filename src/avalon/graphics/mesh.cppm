module;
#include <cstdint>
#include <debug/assert.hpp>
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
  Mesh(BufferHandle vbo, BufferHandle ibo, uint32_t indexCount,
       EFormat indexFormat)
      : m_vertexBuffer(vbo), m_indexBuffer(ibo), m_indexCount(indexCount),
        m_indexFormat(indexFormat) {}

  BufferHandle GetVBO() const noexcept { return m_vertexBuffer; }
  BufferHandle GetIBO() const noexcept { return m_indexBuffer; }
  EFormat GetIndexFormat() const noexcept { return m_indexFormat; }
  uint32_t GetIndexCount() const noexcept { return m_indexCount; }

private:
  BufferHandle m_vertexBuffer;
  BufferHandle m_indexBuffer;
  uint32_t m_indexCount;
  EFormat m_indexFormat = EFormat::R32_Uint;
};

using MeshHandle = Handle<Mesh>;

} // namespace avalon::graphics
