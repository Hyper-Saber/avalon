module;
#include <cstdint>
export module avalon.graphics:types;

import avalon.core;

export namespace avalon::graphics {
struct CameraSnapshot {
  Matrix4x4 view;
  Matrix4x4 projection;
  Vec4 position;
};

struct LightData {
  Vec4 color;       // [R, G, B, Intensity]
  Vec4 dirOrPos;    // [x, y, z, Range]
  uint32_t type;    // 0: Dir, 1: Point, 2: Spot
  float padding[3]; // 保证 16 字节对齐
};
struct SceneGlobals {
  CameraSnapshot camera;
  LightData lightData;
};

enum class EPrimitiveType {
  Cube,
  Plane,
  Quad,
  Sphere,
};
} // namespace avalon::graphics
