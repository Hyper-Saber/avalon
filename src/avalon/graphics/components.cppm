module;
export module avalon.graphics:components;

import avalon.ecs;
import :mesh;
import :material_instance;
export namespace avalon::ecs {

struct RenderComponent {
  graphics::MeshHandle meshHandle;
  graphics::MaterialHandle materialHandle;
  graphics::MaterialInstanceHandle materialInstanceHandle;
  AABB localBounds;
};

} // namespace avalon::ecs
