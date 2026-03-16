module;
export module avalon.graphics:components;

import avalon.ecs;
import :mesh;
export namespace avalon::ecs {

struct MeshComponent {
  graphics::MeshHandle meshHandle;
};

} // namespace avalon::ecs
