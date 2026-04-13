module;
#include <array>
#include <cstdint>
#include <debug/assert.hpp>
export module avalon.graphics:material_instance;
import avalon.core;
import avalon.rhi;
import :material;
import :types;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API MaterialInstance final
    : public mem::AutoDestroyable<MaterialInstance> {
public:
  explicit MaterialInstance(MaterialHandle handle, uint32_t index);

  auto GetParent() const noexcept -> MaterialHandle { return m_materialHandle; }

  auto GetGpuIndex() const noexcept { return m_gpuIndex; }

  auto GetTexture(StringId nameHash) const noexcept -> rhi::TextureHandle {
    return rhi::TextureHandle();
  }

  auto GetDataBlob() const noexcept -> const IBlob & {
    return *m_dataBlob.Get();
  }

  float GetAlphaThreshold() { return m_alphaThreshold; }

  uint32_t GetTextureSlot(StringId nameHash) const {
    auto texture = m_textureMap.Get(nameHash);
    if (texture)
      return texture->pushConstantTextureSlot;
    return kInvalidTextureSlot;
  }

  auto GetTextureSlots() const -> Span<const uint32_t> {
    return {&m_textureSlots[0], m_textureSlots.size()};
  }

  void SetTexture(StringId nameHash, rhi::TextureHandle handle) {
    auto texture = m_textureMap.Get(nameHash);
    if (!texture) {
      Error("[Material]: Texture {} not exsit!", nameHash.Resolve());
      return;
    }

    if (texture->handle != handle) {
      texture->handle = handle;
      m_texturePushPending.PushBack(nameHash);
    }
  }

  template <typename T> void SetProperty(StringId nameHash, const T &value) {
    auto buffer = m_propertyMap.Get(nameHash);
    if (!buffer) {
      Error("[Material]: Property {} not exsit!", nameHash.Resolve());
      return;
    }
    AVALON_ASSERT_MSG(sizeof(T) == buffer->size,
                      String::Format("[Material]: Property {} size mismatch!",
                                     nameHash.Resolve()));
    m_dataBlob->Write(&value, buffer->bufferOffset, buffer->size);
  }

private:
  friend class MaterialManager;

  void ClearDirty() { m_texturePushPending.Clear(); }

  void WriteTextureIndex(StringId nameHash, uint32_t bindlessIndex) {
    auto state = m_textureMap.Get(nameHash);
    m_textureSlots[state->pushConstantTextureSlot] = bindlessIndex;
  }

  auto GetTexturePushPending() const noexcept -> const Array<StringId> & {
    return m_texturePushPending;
  }

  float m_alphaThreshold = 1.0;

  MaterialHandle m_materialHandle;
  uint32_t m_gpuIndex;
  BlobPtr m_dataBlob;
  HashMap<StringId, PropertyState> m_propertyMap;
  HashMap<StringId, TextureState> m_textureMap;

  Array<StringId> m_texturePushPending;

  std::array<uint32_t, 7> m_textureSlots;
};
} // namespace avalon::graphics
