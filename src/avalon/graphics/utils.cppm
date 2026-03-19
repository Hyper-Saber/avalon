module;
export module avalon.graphics:utils;

import avalon.core;
import :mesh_manager;
import :material_manager;

export namespace avalon::graphics {

inline auto GetMeshManager() -> MeshManager & {
  return GetContext().GetService<MeshManager>(EEngineService::MeshManager);
}

inline auto GetMaterialManager() -> MaterialManager & {
  return GetContext().GetService<MaterialManager>(
      EEngineService::MaterialManager);
}

} // namespace avalon::graphics
