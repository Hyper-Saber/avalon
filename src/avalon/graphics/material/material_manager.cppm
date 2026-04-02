module;
#include <cstdint>
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
  bool Initialize() {
    m_freeIndices.Reserve(rhi::kMaxMaterialCount);
    for (uint32_t i = rhi::kMaxMaterialCount - 1; i > 0; i--) {
      m_freeIndices.PushBack(i);
    }
    return true;
  }

  void Update(rhi::IRhi &rhi) {
    auto &bindless = rhi.GetBindlessManager();

    m_materialInstancePool.Foreach([&](MaterialInstance &instance) {
      // if (instance.m_isPropertyDirty) {
      if (true) {

        auto &dataBlob = instance.GetDataBlob();
        uint32_t gpuIdx = instance.GetGpuIndex();
        auto pDataBlob = dataBlob.GetData();
        rhi.UpdateMaterialBuffer(gpuIdx * dataBlob.GetSize(), pDataBlob,
                                 dataBlob.GetSize());
      }

      // if (instance.m_isTextureDirty) {
      //
      //   auto &bindlessManager = rhi.GetBindlessManager();
      //
      //   auto pending = instance.GetTexturePushPending();
      //   for (auto id : pending) {
      //
      //     rhi::TextureHandle texHandle = instance.GetTexture(id);
      //     if (!texHandle.IsValid())
      //       texHandle = rhi.GetDefaultTexture();
      //     uint32_t bindlessIndex =
      //     bindlessManager.RegisterTexture(texHandle);
      //     instance.WriteTextureIndex(id, bindlessIndex);
      //   }
      // }

      instance.ClearDirty();
    });
  }

  auto CreateMaterial(ShaderHandle shader, StringId nameHash)
      -> MaterialHandle {
    if (auto *existing = m_materials.Get(nameHash)) {
      return *existing;
    }

    auto handle = m_materialPool.Create(shader);
    if (handle.IsValid()) {
      m_materials.Insert(nameHash, handle);
    }
    return handle;
  }

  auto TryGetMaterial(StringId nameHash) -> MaterialHandle {
    if (auto *existing = m_materials.Get(nameHash)) {
      return *existing;
    }
    return {};
  }

  auto CreateMaterialInstance(MaterialHandle handle = {})
      -> MaterialInstanceHandle {
    if (!handle.IsValid()) {
      return m_defaultOpaqueInstance;
    }

    AVALON_ASSERT(!m_freeIndices.IsEmpty() && "Out of material indices!");
    uint32_t gpuIdx = m_freeIndices.GetBack();
    m_freeIndices.PopBack();

    auto instanceHandle = m_materialInstancePool.Create(handle, gpuIdx);
    return instanceHandle;
  }

  void DestroyMaterialInstance(MaterialInstanceHandle handle) {
    if (auto *instance = m_materialInstancePool.Resolve(handle)) {
      m_freeIndices.PushBack(instance->GetGpuIndex());
      m_materialInstancePool.Release(handle);
    }
  }

  auto ResolveSafe(MaterialInstanceHandle handle, bool isTransparent = false)
      -> MaterialInstance & {
    auto *instance = m_materialInstancePool.Resolve(handle);
    if (instance)
      return *instance;

    return isTransparent
               ? *m_materialInstancePool.Resolve(m_defaultTransparentInstance)
               : *m_materialInstancePool.Resolve(m_defaultOpaqueInstance);
  }

  auto Resolve(MaterialHandle handle) -> Material * {
    return m_materialPool.Resolve(handle);
  }

  auto Resolve(MaterialInstanceHandle handle) -> MaterialInstance * {
    return m_materialInstancePool.Resolve(handle);
  }

  void SetDefaultOpaque(MaterialHandle handle) {
    m_defaultOpaque = handle;
    if (m_defaultOpaqueInstance.IsValid())
      m_materialInstancePool.Release(m_defaultOpaqueInstance);

    m_defaultOpaqueInstance = CreateMaterialInstance(m_defaultOpaque);
  }

  void SetDefaultTransparent(MaterialHandle handle) {
    m_defaultTransparent = handle;
    if (m_defaultTransparentInstance.IsValid())
      m_materialInstancePool.Release(m_defaultTransparentInstance);
    m_defaultTransparentInstance = CreateMaterialInstance(m_defaultTransparent);
  }

  void SetDefaultBlit(MaterialHandle handle) { m_defaultBlit = handle; }
  void SetDefaultSkyBox(MaterialHandle handle) { m_defaultSkybox = handle; }
  void SetErrorMaterial(MaterialHandle handle) { m_errorMaterial = handle; }

  auto GetDefaultOpaque() const -> MaterialHandle { return m_defaultOpaque; }
  auto GetDefaultTransparent() const -> MaterialHandle {
    return m_defaultTransparent;
  }
  auto GetDefaultBlit() const -> MaterialHandle { return m_defaultBlit; }
  auto GetDefaultSkybox() const -> MaterialHandle { return m_defaultSkybox; }
  auto GetDefaultOpaqueInstance() const -> MaterialInstanceHandle {
    return m_defaultOpaqueInstance;
  }
  auto GetDefaultTransparentInstance() const -> MaterialInstanceHandle {
    return m_defaultTransparentInstance;
  }

private:
  MaterialHandle m_defaultOpaque;
  MaterialHandle m_defaultTransparent;
  MaterialHandle m_defaultBlit;
  MaterialHandle m_defaultSkybox;
  MaterialHandle m_errorMaterial;

  MaterialInstanceHandle m_defaultOpaqueInstance;
  MaterialInstanceHandle m_defaultTransparentInstance;

  HashMap<StringId, MaterialHandle> m_materials;
  mem::ResourcePool<Material> m_materialPool;
  mem::ResourcePool<MaterialInstance> m_materialInstancePool;

  Array<uint32_t> m_freeIndices;
};

} // namespace avalon::graphics
