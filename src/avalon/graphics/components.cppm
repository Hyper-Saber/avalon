module;
export module avalon.graphics:components;

import avalon.ecs;
import :mesh;
export namespace avalon::graphics {

struct MeshComponent : public ecs::IComponent {
  MeshHandle meshHandle;
};

struct TransformComponent : public ecs::IComponent {
  Matrix4x4 transform;
};

} // namespace avalon::graphics
