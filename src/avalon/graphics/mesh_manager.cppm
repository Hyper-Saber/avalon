module;
#include <cstddef>
#include <cstdint>

export module avalon.graphics:mesh_manager;

import :mesh;
import avalon.core;

using namespace avalon::rhi;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API MeshManager final
    : public mem::AutoDestroyable<MeshManager> {
public:
  explicit MeshManager(IRhi &rhi) : m_rhi(rhi) {}
  MeshHandle CreateMesh(const MeshData &data) {
    if (!data.IsValid())
      return {};

    size_t vBufferSize = data.positions.GetSize() * sizeof(float);
    size_t iBufferSize = data.indices.GetSize() * sizeof(uint32_t);

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

    VertexLayout layout{.stride = sizeof(float) * 3};
    layout.attributes.PushBack({0, EFormat::R32G32B32_Float3, 0});

    return m_meshPool.Create(vHandle, iHandle, data.indices.GetSize(),
                             EFormat::R32_Uint, layout);
  }

  Mesh *Resolve(MeshHandle handle) { return m_meshPool.Resolve(handle); }

private:
  IRhi &m_rhi;
  mem::ResourcePool<Mesh> m_meshPool;
};
} // namespace avalon::graphics
