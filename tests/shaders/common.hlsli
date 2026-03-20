#ifndef AVALON_COMMON_HLSLI
#define AVALON_COMMON_HLSLI

#define VK_BINDING(b, s) [[vk::binding(b, s)]]
#define VK_PUSH_CONSTANT [[vk::push_constant]]
#define VK_LOCATION(l) [[vk::location(l)]]

#define uCamera uSceneGlobals.camera
#define uMainLight uSceneGlobals.lightData
#define uTime uSceneGlobals.time.time
#define uSineTime uSceneGlobals.time.sineTime
#define uCosineTime uSceneGlobals.time.cosineTime
#define uDeltaTime uSceneGlobals.time.deltaTime

#define PI 3.14159265

struct Camera {
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

struct GlobalTime {
  float time;
  float sineTime;
  float cosineTime;
  float deltaTime;
};

struct Resolution {
  float width;
  float height;
  float invWidth;
  float invHeight;
};

struct SceneGlobals {
  Camera camera;
  LightData lightData;
  GlobalTime time;
  Resolution resolution;
};

VK_BINDING(0, 0) ConstantBuffer<SceneGlobals> uSceneGlobals
    : register(b0, space0);

struct ModelData {
  float4x4 model;
  float4x4 normalMatrix;
};

#endif // AVALON_COMMON_HLSLI
