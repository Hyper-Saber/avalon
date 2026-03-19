module;
export module avalon.shader:service;

import avalon.core;
import :shader_manager;

export namespace avalon::graphics {
inline auto GetShaderManager() -> ShaderManager & {
  return GetContext().GetService<ShaderManager>(EEngineService::ShaderManager);
}

} // namespace avalon::graphics
