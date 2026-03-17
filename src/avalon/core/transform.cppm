export module avalon.core:transform;

import :math;
import :math.quaternion;

export namespace avalon {

struct Transform {
  Vec3 position = {0.0f, 0.0f, 0.0f};
  Quaternion rotation;
  Vec3 scale = {1.0f, 1.0f, 1.0f};
};

} // namespace avalon
