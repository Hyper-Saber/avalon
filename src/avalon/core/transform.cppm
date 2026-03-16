export module avalon.core:transform;

import :math;

export namespace avalon {

struct Transform {
  Vec3 position = {0.0f, 0.0f, 0.0f};
  Vec3 rotation = {0.0f, 0.0f, 0.0f};
  Vec3 scale = {1.0f, 1.0f, 1.0f};
};

} // namespace avalon
