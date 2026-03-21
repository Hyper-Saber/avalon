module;
#include <debug/assert.hpp>
export module avalon.graphics:material_manager;

import avalon.core;
import avalon.shader;
import :material;
import :material_instance;

export namespace avalon::graphics {

class AVALON_GRAPHICS_API MaterialManager final
    : public mem::AutoDestroyable<MaterialManager> {
public:
  auto CreateMaterial(ShaderHandle shader, StringId nameHash)
      -> MaterialHandle {
    if (m_materials.Contains(nameHash)) {
      return *m_materials.Get(nameHash);
    }

    auto handle = m_materialPool.Create(shader);

    if (!handle.IsValid())
      return {};

    m_materials.Insert(nameHash, handle);
    return handle;
  }

  auto CreateMaterialInstance(MaterialHandle handle = {})
      -> MaterialInstanceHandle {
    if (!handle.IsValid()) {
      AVALON_ASSERT(m_defaultMaterial.IsValid());
      if (!m_defaultMaterialInstance.IsValid())
        m_defaultMaterialInstance =
            m_materialInstancePool.Create(m_defaultMaterial);
      return m_defaultMaterialInstance;
    }
    return m_materialInstancePool.Create(handle);
  }

  void ReleaseMaterialInstance(MaterialInstanceHandle handle) {
    m_materialInstancePool.Resolve(handle);
  }

  void SetDefaultMaterial(MaterialHandle handle) { m_defaultMaterial = handle; }

  auto GetDefaultMaterialInstance() -> MaterialInstanceHandle {
    return m_materialInstancePool.Create(m_defaultMaterial);
  }

  auto Resolve(MaterialHandle handle) -> Material * {
    return m_materialPool.Resolve(handle);
  }

  auto Resolve(MaterialInstanceHandle handle) -> MaterialInstance * {
    return m_materialInstancePool.Resolve(handle);
  }

private:
  MaterialHandle m_defaultMaterial;
  MaterialInstanceHandle m_defaultMaterialInstance;
  HashMap<StringId, MaterialHandle> m_materials;

  mem::ResourcePool<Material> m_materialPool;
  mem::ResourcePool<MaterialInstance> m_materialInstancePool;
};

} // namespace avalon::graphics
