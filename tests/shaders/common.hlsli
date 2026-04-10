#ifndef AVALON_COMMON_HLSLI
#define AVALON_COMMON_HLSLI

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

// ==========================================
// Sample Helpers (Sampled Images / SRV)
// ==========================================

float4 sampleTexture2d(uint index, uint samplerIdx, float2 uv) {
  return uTextures[index].Sample(uSamplers[samplerIdx], uv);
}

float4 sampleTexture2dLod(uint index, uint samplerIdx, float2 uv, float lod) {
  return uTextures[index].SampleLevel(uSamplers[samplerIdx], uv, lod);
}

float4 sampleCube(uint index, uint samplerIdx, float3 dir) {
  return uEnvCubes[index].Sample(uSamplers[samplerIdx], dir);
}

float4 sampleCubeLod(uint index, uint samplerIdx, float3 dir, float lod) {
  return uEnvCubes[index].SampleLevel(uSamplers[samplerIdx], dir, lod);
}

float4 sampleTextureArray(uint index, uint samplerIdx, float3 uvLayer) {
  return uTextureArrays[index].Sample(uSamplers[samplerIdx], uvLayer);
}

float4 sampleTextureArrayLod(uint index, uint samplerIdx, float3 uvLayer,
                             float lod) {
  return uTextureArrays[index].SampleLevel(uSamplers[samplerIdx], uvLayer, lod);
}

float4 sampleVolume(uint index, uint samplerIdx, float3 uvw) {
  return uVolumes[index].Sample(uSamplers[samplerIdx], uvw);
}

// ==========================================
// Load Helpers (Point Fetch / Int Coordinates)
// ==========================================

float4 loadTexture2d(uint index, int2 texelCoord, uint mip = 0) {
  return uTextures[index].Load(int3(texelCoord, mip));
}

float4 loadTextureArray(uint index, int3 texelCoordLayer, uint mip = 0) {
  return uTextureArrays[index].Load(int4(texelCoordLayer, mip));
}

// ==========================================
// Storage Helpers (Read/Write / UAV)
// ==========================================

void writeRWTexture2d(uint index, uint2 coord, float4 value) {
  uRWTextures[index][coord] = value;
}

float4 loadRWTexture2d(uint index, uint2 coord) {
  return uRWTextures[index][coord];
}

void writeRWTextureArray(uint index, uint3 coord, float4 value) {
  uRWTextureArrays[index][coord] = value;
}

float4 loadRWTextureArray(uint index, uint3 coord) {
  return uRWTextureArrays[index][coord];
}

#define ATOMIC_ADD_FLOAT(buffer, addr, val)                                    \
  do {                                                                         \
    uint _actual;                                                              \
    uint _expected;                                                            \
    _actual = buffer.Load(addr);                                               \
    [loop] for(int _i = 0; _i < 64; ++_i) {                                    \
      _expected = _actual;                                                     \
      float _newVal = asfloat(_expected) + (val);                              \
      buffer.InterlockedCompareExchange(addr, _expected, asuint(_newVal),      \
                                        _actual);                              \
      if(_actual == _expected)                                                 \
        break;                                                                 \
    }                                                                          \
  } while(0)

static const float3 kFaceForwards[6] = {
    float3(1.0, 0.0, 0.0),  // +X
    float3(-1.0, 0.0, 0.0), // -X
    float3(0.0, 1.0, 0.0),  // +Y
    float3(0.0, -1.0, 0.0), // -Y
    float3(0.0, 0.0, 1.0),  // +Z
    float3(0.0, 0.0, -1.0)  // -Z
};

static const float3 kFaceRights[6] = {
    float3(0.0, 0.0, -1.0), // +X 边缘接 -Z
    float3(0.0, 0.0, 1.0),  // -X 边缘接 +Z
    float3(1.0, 0.0, 0.0),  // +Y 边缘接 +X
    float3(1.0, 0.0, 0.0),  // -Y 边缘接 +X
    float3(1.0, 0.0, 0.0),  // +Z 边缘接 +X
    float3(-1.0, 0.0, 0.0)  // -Z 边缘接 -X
};

static const float3 kFaceUps[6] = {
    float3(0.0, -1.0, 0.0), // +X
    float3(0.0, -1.0, 0.0), // -X
    float3(0.0, 0.0, 1.0),  // +Y
    float3(0.0, 0.0, -1.0), // -Y
    float3(0.0, -1.0, 0.0), // +Z
    float3(0.0, -1.0, 0.0)  // -Z
};

float3 getCubemapDirection(float2 uv, uint faceIndex) {
  float2 st = uv * 2.0 - 1.0;

  float3 f = kFaceForwards[faceIndex];
  float3 r = kFaceRights[faceIndex];
  float3 u = kFaceUps[faceIndex];

  return normalize(f + st.x * r + st.y * u);
}

#endif // AVALON_COMMON_HLSLI
