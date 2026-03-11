module;
#include <cstdint>
export module avalon.graphics:mesh;

import avalon.core;
import avalon.rhi;

using namespace avalon::rhi;

namespace avalon::graphics {

struct VertexAttribute {
  uint32_t location;
  rhi::EFormat format;
  uint32_t offset;
};

struct VertexLayout {
  uint32_t stride;
  Array<VertexAttribute> attributes;
};

struct MeshData {
  struct SubMesh {
    StringId nameHash;
    uint32_t baseVertex;
    uint32_t startIndex;
    uint32_t indexCount;
  };

  Array<float> positions;
  Array<float> normals;
  Array<float> texCoords;
  Array<uint32_t> indices;
  Array<SubMesh> subMeshs;

  struct AABB {
    float min[3];
    float max[3];
  } bounds;

  bool IsValid() const noexcept {
    return !positions.IsEmpty() && !indices.IsEmpty();
  }
};

class AVALON_GRAPHICS_API Mesh final : public mem::AutoDestroyable<Mesh> {
public:
  Mesh(BufferHandle vbo, BufferHandle ibo, uint32_t indexCount,
       EFormat indexFormat, const VertexLayout &layout)
      : m_vertexBuffer(vbo), m_indexBuffer(ibo), m_indexCount(indexCount),
        m_indexFormat(indexFormat), m_layout(layout) {}

  BufferHandle GetVBO() const noexcept { return m_vertexBuffer; }
  BufferHandle GetIBO() const noexcept { return m_indexBuffer; }
  EFormat GetIndexFormat() const noexcept { return m_indexFormat; }
  auto GetLayout() const -> const VertexLayout & { return m_layout; }
  uint32_t GetIndexCount() const noexcept { return m_indexCount; }

private:
  BufferHandle m_vertexBuffer;
  BufferHandle m_indexBuffer;
  uint32_t m_indexCount;
  EFormat m_indexFormat = EFormat::R32_Uint;
  VertexLayout m_layout;
};

using MeshHandle = Handle<Mesh>;

} // namespace avalon::graphics
