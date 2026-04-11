#ifndef AVALON_STANDARD_LAYOUT_HLSLI
#define AVALON_STANDARD_LAYOUT_HLSLI

#include "constants.hlsli"

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

struct Camera {
  float4x4 view;
  float4x4 projection;
  float4x4 viewProjection;
  float4x4 invView;
  float4x4 invProjection;
  float4x4 invViewProjection;
  float4 worldPosition;
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

struct CubemapSH {
  float4 coefficients[9];
};

struct SceneGlobals {
  Camera camera;
  Light light;
  GlobalTime time;
  float4 resolution;
  CubemapSH skyboxSH;
};

struct MaterialData {
  float4 albedo;
  float metallic;
  float roughness;
  float ao;
  float emissive;

  uint albedoTex;
  uint normalTex;
  uint pbrTex;
  uint sampler;
};

struct ProbeData {
  float4x4 captureViews[6];
};

#ifndef CUSTOM_PUSH_TYPE

#define mModel push.model.model
#define mNormalMatrix push.model.normalMatrix
#define mMaterialIndex push.materialIdx

#define mMaterial uMaterials[mMaterialIndex]

struct ModelData {
  float4x4 model;
  float4x4 normalMatrix;
};

struct StandardPushConstants {
  ModelData model;
  uint materialIdx;
  uint skyboxSHOffset;
  uint prefilterMap;
  uint brdfLut;
  uint paddings[kPushConstantFloatSize - 36];
};

#define CUSTOM_PUSH_TYPE StandardPushConstants
#define STANDARD_PUSH

#endif

VK_PUSH_CONSTANT CUSTOM_PUSH_TYPE push;

VK_BINDING(0, 0) SamplerState uSamplers[] : register(s0, space0);
VK_BINDING(1, 0) StructuredBuffer<MaterialData> uMaterials
    : register(t0, space0);
VK_BINDING(2, 0) StructuredBuffer<ProbeData> uProbes : register(t1, space0);
VK_BINDING(3, 0) TextureCube uEnvCubes[] : register(t8, space0);
VK_BINDING(4, 0) Texture2DArray uTextureArrays[] : register(t128, space0);
VK_BINDING(5, 0) Texture3D uVolumes[] : register(t256, space0);
VK_BINDING(6, 0) Texture2D uTextures[] : register(t512, space0);
VK_BINDING(7, 0) RWTexture2D<float4> uRWTextures[] : register(u0, space0);
VK_BINDING(8, 0) RWByteAddressBuffer uGeneralSSBO : register(u1024, space0);
VK_BINDING(9, 0) RWTexture2DArray<float4> uRWTextureArrays[]
    : register(u2048, space0);

VK_BINDING(0, 1) ConstantBuffer<SceneGlobals> uSceneGlobals
    : register(b0, space1);

struct StandardVSInput {
  VK_LOCATION(0) float3 position : POSITION;
  VK_LOCATION(1) float3 color : COLOR;
  VK_LOCATION(2) float2 uv : TEXCOORD0;
  VK_LOCATION(3) float3 normal : NORMAL;
};

#ifdef STANDARD_PUSH
float4 calculateClipPosition(float4 worldPos) {
  return mul(uCamera.viewProjection, worldPos);
}
#endif

#endif
