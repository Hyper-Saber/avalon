module;
export module avalon.graphics:utils;

import avalon.core;
import avalon.shader;
import :mesh_manager;

export namespace avalon::graphics {
inline auto GetShaderManager() -> ShaderManager & {
  return GetContext().GetService<ShaderManager>(EEngineService::ShaderManager);
}

inline auto GetMeshManager() -> MeshManager & {
  return GetContext().GetService<MeshManager>(EEngineService::MeshManager);
}
} // namespace avalon::graphics
