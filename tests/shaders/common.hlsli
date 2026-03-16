#ifndef AVALON_COMMON_HLSLI
#define AVALON_COMMON_HLSLI

#define VK_BINDING(b, s) [[vk::binding(b, s)]]
#define VK_PUSH_CONSTANT [[vk::push_constant]]
#define VK_LOCATION(l) [[vk::location(l)]]

struct SceneGlobals {
  float4x4 view;
  float4x4 projection;
  float4 cameraPosition;
};

struct LightData {
  float4 color;    // [R, G, B, Intensity]
  float4 dirOrPos; // [x, y, z, Range]
  uint type;       // 0: Dir, 1: Point, 2: Spot
  float3 padding;  // 保证 16 字节对齐
};

VK_BINDING(0, 0) ConstantBuffer<SceneGlobals> uSceneGlobals
    : register(b0, space0);
VK_BINDING(1, 0) ConstantBuffer<LightData> uMainLight : register(b1, space0);

struct ModelData {
  float4x4 model;
};

#endif // AVALON_COMMON_HLSLI
