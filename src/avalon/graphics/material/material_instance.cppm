module;
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
  explicit MaterialInstance(MaterialHandle handle);

  template <typename T>
  void SetProperty(StringId nameHash, const T &value) noexcept {
    const auto &layout = GetPropertyLayout();
    for (const auto &mapping : layout) {
      if (mapping.nameHash == nameHash) {
        UpdateInternal(mapping, value);
        return;
      }
    }
  }

  auto GetMaterialHandle() const noexcept -> MaterialHandle {
    return m_materialHandle;
  }

  auto GetBufferStates() const noexcept -> const Array<UniformBufferState> & {
    return m_instanceBuffers;
  }

  auto GetDataBlob() const noexcept -> const IBlob & {
    return *m_dataBlob.Get();
  }

private:
  template <typename T>
  void UpdateInternal(const PropertyMapping &mapping, const T &value) {
    AVALON_ASSERT_MSG(sizeof(T) == mapping.size, "Property size mismatch!");

    auto &state = m_instanceBuffers[mapping.bufferIndex];
    state.isDirty = m_dataBlob->Write(
        &value, state.bufferOffset + mapping.memberOffset, mapping.size);
  }

  auto GetPropertyLayout() const noexcept -> const Array<PropertyMapping> &;

  MaterialHandle m_materialHandle;
  Array<UniformBufferState> m_instanceBuffers;
  BlobPtr m_dataBlob;
};

using MaterialInstanceHandle = Handle<MaterialInstance>;
} // namespace avalon::graphics
