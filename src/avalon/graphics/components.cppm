module;
export module avalon.graphics:components;

import avalon.ecs;
import :mesh;
export namespace avalon::graphics {

struct MeshComponent {
  MeshHandle meshHandle;
};

struct TransformComponent {
  Transform local;
};

} // namespace avalon::graphics
