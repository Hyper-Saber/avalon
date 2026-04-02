#ifndef AVALON_COMMON_HLSLI
#define AVALON_COMMON_HLSLI

#define VK_BINDING(b, s) [[vk::binding(b, s)]]
#define VK_PUSH_CONSTANT [[vk::push_constant]]
#define VK_LOCATION(l) [[vk::location(l)]]

#define uCamera uSceneGlobals.camera
#define uMainLight uSceneGlobals.light
#define uTime uSceneGlobals.time.time
#define uSineTime uSceneGlobals.time.sineTime
#define uCosineTime uSceneGlobals.time.cosineTime
#define uDeltaTime uSceneGlobals.time.deltaTime
#define uResolution uSceneGlobals.resolution

#define mModel push.model.model
// #define mModelView push.model.modelView
// #define mMVP push.model.mvp
#define mNormalMatrix push.model.normalMatrix
#define mMaterialIndex push.materialIdx

#define mMaterial uMaterials[mMaterialIndex]

#define kPi 3.14159265359
#define kEpsilon 1e-6

struct Camera {
  float4x4 view;
  float4x4 projection;
  float4x4 viewProjection;
  float4x4 invView;
  float4x4 invProjection;
  float4x4 invViewProjection;
  float4 cameraPosition;
};

struct Light {
  float4 colorIntensity;
  float4 posDir;
  uint type;
  float range;
  float spotInnerCosine;
  float spotOuterCosine;
};

struct GlobalTime {
  float time;
  float sineTime;
  float cosineTime;
  float deltaTime;
};

struct SceneGlobals {
  Camera camera;
  Light light;
  GlobalTime time;
  float4 resolution;
};

struct MaterialData {
  float4 baseColor;
  float4 specularColor;
  float shininess;
  float f0;
  float2 padding;
};

struct ModelData {
  float4x4 model;
  float4x4 normalMatrix;
};

struct StandardPushConstants {
  ModelData model;
  uint materialIdx;
  uint textureIndices[7];
};

VK_BINDING(0, 0) StructuredBuffer<MaterialData> uMaterials
    : register(t2, space0);
VK_BINDING(1, 0) Texture2D uTextures[] : register(t0, space0);
VK_BINDING(2, 0) SamplerState uSamplers[] : register(s0, space0);

VK_BINDING(0, 1) ConstantBuffer<SceneGlobals> uSceneGlobals
    : register(b0, space1);

float4 sampleBindless(uint textureIdx, uint samplerIdx, float2 uv) {
  return uTextures[textureIdx].Sample(uSamplers[samplerIdx], uv);
}

#endif // AVALON_COMMON_HLSLI
