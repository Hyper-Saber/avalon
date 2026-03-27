module;
export module avalon.graphics:types;

import avalon.core;
import avalon.rhi;

export namespace avalon::graphics {

class Mesh;
class Material;
class MaterialInstance;

constexpr StringId kSwapchainColor = "SwapchainColor"_id;
constexpr StringId kSceneGlobalsBuffer = "uSceneGlobals"_id;
constexpr StringId kSceneColor = "SceneColor"_id;
constexpr StringId kSceneDepth = "SceneDepth"_id;

using MeshHandle = Handle<Mesh>;
using MaterialHandle = Handle<Material>;
using MaterialInstanceHandle = Handle<MaterialInstance>;

struct Plane {
  Vec3 normal;
  float distance;

  float GetSignedDistance(const Vec3 &point) const {
    return normal.Dot(point) + distance;
  }

  String ToString() {
    return String::Format("Plane(normal={}, distance={})", normal.ToString(),
                          distance);
  }
};

struct Frustum {
  Plane planes[6];

  void Update(const Matrix4x4 &vp) {
    planes[0].normal = {vp[3][0] + vp[0][0], vp[3][1] + vp[0][1],
                        vp[3][2] + vp[0][2]};
    planes[0].distance = vp[3][3] + vp[0][3];

    planes[1].normal = {vp[3][0] - vp[0][0], vp[3][1] - vp[0][1],
                        vp[3][2] - vp[0][2]};
    planes[1].distance = vp[3][3] - vp[0][3];

    planes[2].normal = {vp[3][0] - vp[1][0], vp[3][1] - vp[1][1],
                        vp[3][2] - vp[1][2]};
    planes[2].distance = vp[3][3] - vp[1][3];

    planes[3].normal = {vp[3][0] + vp[1][0], vp[3][1] + vp[1][1],
                        vp[3][2] + vp[1][2]};
    planes[3].distance = vp[3][3] + vp[1][3];

    planes[4].normal = {vp[3][0] - vp[2][0], vp[3][1] - vp[2][1],
                        vp[3][2] - vp[2][2]};
    planes[4].distance = vp[3][3] - vp[2][3];

    planes[5].normal = {-vp[2][0], -vp[2][1], -vp[2][2]};
    planes[5].distance = -vp[2][3];

    for (int i = 0; i < 6; ++i) {
      float length = planes[i].normal.Length();
      if (length > kEpsilon) {
        float invLen = 1.0f / length;
        planes[i].normal *= invLen;
        planes[i].distance *= invLen;
      }
    }
  }

  bool IsVisible(const AABB &aabb) const {
    Vec3 center = (aabb.max + aabb.min) * 0.5f;
    Vec3 extents = (aabb.max - aabb.min) * 0.5f;

    for (int i = 0; i < 6; ++i) {
      const auto &plane = planes[i];
      float r = extents.x * Abs(plane.normal.x) +
                extents.y * Abs(plane.normal.y) +
                extents.z * Abs(plane.normal.z);

      if (plane.GetSignedDistance(center) < -r) {
        return false;
      }
    }
    return true;
  }

  String ToString() {
    return String::Format("Frustum:\n Left: {}\n Right: {}\n Bottom: {}\n Top: "
                          "{}\n Near: {}\n Far: {}",
                          planes[0].ToString(), planes[1].ToString(),
                          planes[2].ToString(), planes[3].ToString(),
                          planes[4].ToString(), planes[5].ToString());
  }
};

enum class EPrimitiveType {
  Cube,
  Plane,
  Quad,
  Sphere,
};

} // namespace avalon::graphics
