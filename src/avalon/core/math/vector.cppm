module;
export module avalon.core:math.vector;

import :string;

export namespace avalon {

struct Vec2 {
  float x = 0, y = 0;
  auto ToString() { return String::Format("x: {}, y: {}", x, y); }
};

struct Vec3 {
  float x = 0, y = 0, z = 0;

  Vec3 operator+(const Vec3 &v) const { return {x + v.x, y + v.y, z + v.z}; }
  Vec3 operator-(const Vec3 &v) const { return {x - v.x, y - v.y, z - v.z}; }
  Vec3 operator+(float s) const { return {x + s, y + s, z + s}; }
  Vec3 operator-(float s) const { return {x - s, y - s, z - s}; }
  Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
  Vec3 operator+=(float s) {
    x += s;
    y += s;
    z += s;
    return *this;
  }
  Vec3 operator-=(float s) {
    x -= s;
    y -= s;
    z -= s;
    return *this;
  }
  Vec3 operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }

  Vec3 Cross(const Vec3 &v) const {
    return Vec3{
        y * v.z - z * v.y,
        z * v.x - x * v.z,
        x * v.y - y * v.x,
    };
  }

  static constexpr Vec3 Cross(const Vec3 &a, const Vec3 &b) noexcept {
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
  }

  static Vec3 Right() { return {1.0f, 0.0f, 0.0f}; }
  static Vec3 Left() { return {-1.0f, 0.0f, 0.0f}; }
  static Vec3 Up() { return {0.0f, 1.0f, 0.0f}; }
  static Vec3 Down() { return {0.0f, -1.0f, 0.0f}; }
  static Vec3 Backward() { return {0.0f, 0.0f, 1.0f}; }
  static Vec3 Forward() { return {0.0f, 0.0f, -1.0f}; }

  static Vec3 Zero() { return {0.0f, 0.0f, 0.0f}; }
  static Vec3 One() { return {1.0f, 1.0f, 1.0f}; }

  auto ToString() { return String::Format("x: {}, y: {}, z: {}", x, y, z); }
};

struct Vec4 {
  float x = 0, y = 0, z = 0, w = 1;

  static Vec4 FromVec3(const Vec3 &v, float w = 1.0f) {
    return {v.x, v.y, v.z, w};
  }

  auto ToString() {
    return String::Format("x: {}, y: {}, z: {}, w: {}", x, y, z, w);
  }
};

} // namespace avalon
