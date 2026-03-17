module;
#include <cmath>
#include <cstdint>
export module avalon.graphics:primitive_generator;

import :mesh;

namespace avalon::graphics {
class PrimitiveGenerator {
public:
  static MeshData GenerateCube() {
    return {
        .positions{// Front (Z=0.5)
                   {-0.5f, -0.5f, 0.5f},
                   {0.5f, -0.5f, 0.5f},
                   {0.5f, 0.5f, 0.5f},
                   {-0.5f, 0.5f, 0.5f},
                   // Back (Z=-0.5)
                   {-0.5f, -0.5f, -0.5f},
                   {-0.5f, 0.5f, -0.5f},
                   {0.5f, 0.5f, -0.5f},
                   {0.5f, -0.5f, -0.5f},
                   // Right (X=0.5)
                   {0.5f, -0.5f, 0.5f},
                   {0.5f, -0.5f, -0.5f},
                   {0.5f, 0.5f, -0.5f},
                   {0.5f, 0.5f, 0.5f},
                   // Left (X=-0.5)
                   {-0.5f, -0.5f, 0.5f},
                   {-0.5f, 0.5f, 0.5f},
                   {-0.5f, 0.5f, -0.5f},
                   {-0.5f, -0.5f, -0.5f},
                   // Top (Y=0.5)
                   {-0.5f, 0.5f, 0.5f},
                   {0.5f, 0.5f, 0.5f},
                   {0.5f, 0.5f, -0.5f},
                   {-0.5f, 0.5f, -0.5f},
                   // Bottom (Y=-0.5)
                   {0.5f, -0.5f, 0.5f},
                   {-0.5f, -0.5f, 0.5f},
                   {-0.5f, -0.5f, -0.5f},
                   {0.5f, -0.5f, -0.5f}},
        .indices{
            0,  1,  2,  2,  3,  0,  // Front
            4,  5,  6,  6,  7,  4,  // Back
            8,  9,  10, 10, 11, 8,  // Right
            12, 13, 14, 14, 15, 12, // Left
            16, 17, 18, 18, 19, 16, // Top
            20, 21, 22, 22, 23, 20  // Bottom
        },
        .colors{
            {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, // Front
            {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, // Back
            {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, // Right
            {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, // Left
            {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, // Top
            {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}  // Bottom
        },
        .normals{
            {0, 0, 1},  {0, 0, 1},  {0, 0, 1},  {0, 0, 1},  // Front
            {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, // Back
            {1, 0, 0},  {1, 0, 0},  {1, 0, 0},  {1, 0, 0},  // Right
            {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, // Left
            {0, 1, 0},  {0, 1, 0},  {0, 1, 0},  {0, 1, 0},  // Top
            {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}  // Bottom
        },
        .texCoords{
            {0, 0}, {1, 0}, {1, 1}, {0, 1}, // Front
            {1, 0}, {0, 0}, {0, 1}, {1, 1}, // Back
            {0, 0}, {1, 0}, {1, 1}, {0, 1}, // Right
            {1, 0}, {0, 0}, {0, 1}, {1, 1}, // Left
            {0, 1}, {1, 1}, {1, 0}, {0, 0}, // Top
            {0, 0}, {1, 0}, {1, 1}, {0, 1}  // Bottom
        },
    };
  }

  static MeshData GeneratePlane() {
    return {
        .positions{
            {-0.5f, 0.0f, 0.5f},
            {0.5f, 0.0f, 0.5f},
            {0.5f, 0.0f, -0.5f},
            {-0.5f, 0.0f, -0.5f},
        },
        .indices{0, 1, 2, 2, 3, 0},
        .colors{
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1},
        },
        .normals{
            {0, 1, 0},
            {0, 1, 0},
            {0, 1, 0},
            {0, 1, 0},
        },
        .texCoords{
            {0, 0},
            {1, 0},
            {1, 1},
            {0, 1},
        },
    };
  }

  static MeshData GenerateQuad() {
    return {
        .positions{
            {-0.5f, -0.5f, 0.0f},
            {0.5f, -0.5f, 0.0f},
            {0.5f, 0.5f, 0.0f},
            {-0.5f, 0.5f, 0.0f},
        },
        .indices{0, 1, 2, 2, 3, 0},
        .colors{
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1},
        },
        .normals{
            {0, 0, 1},
            {0, 0, 1},
            {0, 0, 1},
            {0, 0, 1},
        },
        .texCoords{
            {0, 0},
            {1, 0},
            {1, 1},
            {0, 1},
        },
    };
  }

  static MeshData GenerateSphere() {
    MeshData data;
    constexpr uint32_t segments = 32;

    for (uint32_t y = 0; y <= segments; ++y) {
      for (uint32_t x = 0; x <= segments; ++x) {
        float xSegment = (float)x / segments;
        float ySegment = (float)y / segments;

        float xPos = std::cos(xSegment * 2.0f * kPi) * std::sin(ySegment * kPi);
        float yPos = std::cos(ySegment * kPi);
        float zPos = std::sin(xSegment * 2.0f * kPi) * std::sin(ySegment * kPi);

        data.positions.PushBack({xPos * 0.5f, yPos * 0.5f, zPos * 0.5f});
        data.normals.PushBack({xPos, yPos, zPos});
        data.texCoords.PushBack({xSegment, ySegment});
        data.colors.PushBack({1.0f, 1.0f, 1.0f});
      }
    }

    for (uint32_t y = 0; y < segments; ++y) {
      for (uint32_t x = 0; x < segments; ++x) {
        uint32_t first = (y * (segments + 1)) + x;
        uint32_t second = first + segments + 1;

        data.indices.PushBack(first);
        data.indices.PushBack(second);
        data.indices.PushBack(first + 1);

        data.indices.PushBack(second);
        data.indices.PushBack(second + 1);
        data.indices.PushBack(first + 1);
      }
    }
    return data;
  }
};

} // namespace avalon::graphics
