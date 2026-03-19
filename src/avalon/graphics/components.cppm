module;
export module avalon.graphics:components;

import avalon.ecs;
import :mesh;
import :material_instance;
export namespace avalon::ecs {

struct RenderComponent {
  graphics::MeshHandle meshHandle;
  graphics::MaterialInstanceHandle materialInstanceHandle;
};

} // namespace avalon::ecs
