module;
#include <cstdint>

export module avalon.core:types;
import :math.vector;
import :math.matrix;
import :handle;
import :containers.array;

export namespace avalon {

struct AABB {
  Vec3 min;
  Vec3 max;

  AABB Transform(const Matrix4x4 &matrix) const {
    Vec3 newMin = matrix.GetTranslation();
    Vec3 newMax = newMin;

    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        float a = matrix[i][j] * min[j];
        float b = matrix[i][j] * max[j];

        if (a < b) {
          newMin[i] += a;
          newMax[i] += b;
        } else {
          newMin[i] += b;
          newMax[i] += a;
        }
      }
    }

    return {newMin, newMax};
  }

  // 将一个新点合并进当前包围盒
  void Encapsulate(const Vec3 &point) {
    if (point.x < min.x)
      min.x = point.x;
    if (point.y < min.y)
      min.y = point.y;
    if (point.z < min.z)
      min.z = point.z;

    if (point.x > max.x)
      max.x = point.x;
    if (point.y > max.y)
      max.y = point.y;
    if (point.z > max.z)
      max.z = point.z;
  }
};

struct Color {
  float r = 0, g = 0, b = 0, a = 1;

  constexpr Color() = default;
  constexpr Color(float r, float g, float b, float a = 1.0f)
      : r(r), g(g), b(b), a(a) {}

  constexpr Vec4 ToVec4() const { return {r, g, b, a}; }
  constexpr Vec3 ToVec3() const { return {r, g, b}; }

  static constexpr Color White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
  static constexpr Color Black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
  static constexpr Color Transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

  static constexpr Color Red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
  static constexpr Color Green() { return {0.0f, 1.0f, 0.0f, 1.0f}; }
  static constexpr Color Blue() { return {0.0f, 0.0f, 1.0f, 1.0f}; }
  static constexpr Color Yellow() { return {1.0f, 1.0f, 0.0f, 1.0f}; }
  static constexpr Color Cyan() { return {0.0f, 1.0f, 1.0f, 1.0f}; }
  static constexpr Color Magenta() { return {1.0f, 0.0f, 1.0f, 1.0f}; }

  static constexpr Color Gray() { return {0.5f, 0.5f, 0.5f, 1.0f}; }
  static constexpr Color DarkGray() { return {0.25f, 0.25f, 0.25f, 1.0f}; }
  static constexpr Color LightGray() { return {0.75f, 0.75f, 0.75f, 1.0f}; }

  static constexpr Color Lerp(const Color &a, const Color &b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t};
  }
};

} // namespace avalon
export namespace avalon::window {

enum class NativeWindowApi {
  Wayland,
  Xcb,
  Win32,
};

struct NativeWindowInfo {
  NativeWindowApi api;

  union {
    struct {
      void *display;
      void *surface;
    } wayland;
    struct {
      void *connection;
      uint32_t window;
    } xcb;
    struct {
      void *hwnd;
      void *hinstance;
    };
  };
};
} // namespace avalon::window

export namespace avalon::rhi {
enum class RenderBackend {
  Auto,
  Vulkan,
  // D3D12, Metal
};

RenderBackend backend = RenderBackend::Auto;
bool enableValidationLayer = true;
} // namespace avalon::rhi

export namespace avalon::graphics {

struct RenderExtensions {
  StringId sType;
  void *pNext = nullptr;
};

struct LightData {
  Vec4 colorIntensity;
  union {
    Vec4 position;
    Vec4 direction;
  } posDir;
  uint32_t type;
  float range;
  float spotInnerCosine;
  float spotOuterCosine;

  void SetColor(const Vec3 &rgb, float intensity) {
    colorIntensity = Vec4::FromVec3(rgb, intensity);
  }

  void SetDirection(const Vec3 &dir) {
    posDir.direction = Vec4::FromVec3(dir.Normalized(), 0.0f);
  }

  void SetPosition(const Vec3 &pos) {
    posDir.position = Vec4::FromVec3(pos, 1.0f);
  }

  float GetIntensity() const { return colorIntensity.w; }
};

struct RenderView {
  Matrix4x4 view;
  Matrix4x4 projection;
  Matrix4x4 viewProjection;
  Matrix4x4 invView;
  Matrix4x4 invProjection;
  Matrix4x4 invViewProjection;
  Vec4 worldPosition;
};

struct SceneSnapshot {
  RenderView camera{};
  LightData lightData{.colorIntensity = {0, 0, 0, 0},
                      .posDir = {.direction = {0, -1, -1, 0}},
                      .type = 0};
  Array<ResourceHandle> opaqueMeshHandles;
  Array<ResourceHandle> opaqueMaterials;
  Array<ResourceHandle> opaqueMaterialInstances;
  Array<Matrix4x4> opaqueWorldMatrices;

  const void *pNext = nullptr;
};

} // namespace avalon::graphics
