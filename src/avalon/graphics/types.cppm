module;

export module avalon.graphics:types;
import avalon.core;

export namespace avalon::graphics {
struct CameraSnapshot {
  const Matrix4x4 view;
  const Matrix4x4 &projection;
  const Vec3 position;
};

struct SceneGlobals {
  Matrix4x4 view;
  Matrix4x4 projection;
  Vec4 cameraPosition;
};

} // namespace avalon::graphics
