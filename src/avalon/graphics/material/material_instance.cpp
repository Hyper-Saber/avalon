module;
#include <utility>
module avalon.graphics;

import :utils;
import :material_instance;

namespace avalon::graphics {

auto MaterialInstance::GetPropertyLayout() const noexcept
    -> const Array<PropertyMapping> & {
  auto material = GetMaterialManager().Resolve(m_materialHandle);
  return material->GetPropertyLayout();
}

MaterialInstance::MaterialInstance(MaterialHandle handle)
    : m_materialHandle(handle) {
  auto material = GetMaterialManager().Resolve(m_materialHandle);
  auto &buffers = material->GetInitialBufferStates();
  m_instanceBuffers = buffers;

  auto &blob = material->GetDataBlob();
  auto data = Array<std::byte>(blob.ConstAs<std::byte>(), blob.GetSize());
  m_dataBlob = CreateBlob(std::move(data));
}

} // namespace avalon::graphics
