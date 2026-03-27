module;
#include <cstdint>
#include <debug/assert.hpp>
#include <utility>
module avalon.graphics;

import :utils;
import :material_instance;

namespace avalon::graphics {

MaterialInstance::MaterialInstance(MaterialHandle handle, uint32_t index)
    : m_materialHandle(handle) {
  auto material = GetMaterialManager().Resolve(m_materialHandle);
  AVALON_ASSERT(material);
  m_gpuIndex = index;
  m_propertyMap = material->GetPropertyLayout();
  m_textureMap = material->GetTextureLayout();

  auto &blob = material->GetDataBlob();
  auto data = Array<std::byte>(blob.ConstAs<std::byte>(), blob.GetSize());
  m_dataBlob = CreateBlob(std::move(data));
}

} // namespace avalon::graphics
