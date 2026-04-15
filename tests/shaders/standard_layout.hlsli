#ifndef AVALON_STANDARD_LAYOUT_HLSLI
#define AVALON_STANDARD_LAYOUT_HLSLI

#include "constants.hlsli"

#define VK_BINDING(b, s) [[vk::binding(b, s)]]
#define VK_PUSH_CONSTANT [[vk::push_constant]]
#define VK_LOCATION(l) [[vk::location(l)]]

#ifndef __COMPUTE_SHADER__
#define STORAGE_BUFFER(type) type
#define STORAGE_STRUCT(type) StructuredBuffer<type>
#define STORAGE_TEXTURE(type) Texture2D<type>
#define STORAGE_TEXTURE_ARRAY(type) Texture2DArray<type>
#define REG_SLOT(t_reg, u_reg) t_reg
#else
#define STORAGE_BUFFER(type) RW##type
#define STORAGE_STRUCT(type) RWStructuredBuffer<type>
#define STORAGE_TEXTURE(type) RWTexture2D<type>
#define STORAGE_TEXTURE_ARRAY(type) RWTexture2DArray<type>
#define REG_SLOT(t_reg, u_reg) u_reg
#endif

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

struct Resolution {
  uint width;
  uint height;
  float invWidth;
  float invHeight;
};

struct SceneGlobals {
  Camera camera;
  Light light;
  GlobalTime time;
  Resolution resolution;
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

struct ModelData {
  float4x4 model;
  float4x4 normalMatrix;
};

struct StandardPushConstants {
  uint instanceBufferOffset;
  uint skyboxSHOffset;
  uint prefilterMap;
  uint brdfLut;

  uint shadowMask;
  uint paddings[kPushConstantFloatSize - 5];
};

#define CUSTOM_PUSH_TYPE StandardPushConstants
#define STANDARD_PUSH

#endif

struct DrawCommand {
  uint indexCount;
  uint instanceCount;
  uint firstIndex;
  int vertexOffset;
  uint firstInstance;
};

struct InstanceData {
  uint instanceID;
  uint materialID;
  uint geometryOffset;
  uint attributesOffset;

  uint indexOffset;
  uint vertexCount;
  uint indexCount;
  uint modelOffset;

  uint invModelOffset;
  uint sdfType;
  uint sdfTextureIndex;
  float alphaThreshold;

  float3 sdfExtent;
  float padding;
};

static const uint kCube = 0;
static const uint kPlane = 1;
static const uint kQuad = 2;
static const uint kSphere = 3;
static const uint kMesh = 4;

struct VertexGeometry {
  float3 position;
  float3 normal;
  float2 uv;
};

struct VertexAttributes {
  float3 color;
};

#define kVertexGeometrySize 32
#define kVertexAttributesSize 12
#define kInstanceDataSize 64

VK_PUSH_CONSTANT CUSTOM_PUSH_TYPE push;

VK_BINDING(0, 0) SamplerState uSamplers[] : register(s0, space0);
VK_BINDING(1, 0) StructuredBuffer<MaterialData> uMaterials
    : register(t0, space0);
VK_BINDING(2, 0) STORAGE_BUFFER(ByteAddressBuffer) uStaticSSBO
    : register(REG_SLOT(t1, u0), space0);
VK_BINDING(3, 0) STORAGE_BUFFER(ByteAddressBuffer) uDynamicSSBO
    : register(REG_SLOT(t2, u1), space0);
VK_BINDING(4, 0) STORAGE_BUFFER(ByteAddressBuffer) uGeometrySSBO
    : register(REG_SLOT(t3, u2), space0);
VK_BINDING(5, 0) STORAGE_BUFFER(ByteAddressBuffer) uAttributesSSBO
    : register(REG_SLOT(t4, u3), space0);
VK_BINDING(6, 0) STORAGE_BUFFER(ByteAddressBuffer) uIndicesSSBO
    : register(REG_SLOT(t5, u4), space0);
VK_BINDING(7, 0) STORAGE_STRUCT(DrawCommand) uCommandSSBO
    : register(REG_SLOT(t6, u5), space0);

VK_BINDING(8, 0) TextureCube uEnvCubes[] : register(t8, space0);
VK_BINDING(9, 0) Texture2DArray uTextureArrays[] : register(t128, space0);
VK_BINDING(10, 0) Texture3D uVolumes[] : register(t256, space0);
VK_BINDING(11, 0) Texture2D uTextures[] : register(t512, space0);

VK_BINDING(12, 0) STORAGE_TEXTURE(float4) uRWTextures[]
    : register(REG_SLOT(t20000, u8), space0);
VK_BINDING(13, 0) STORAGE_TEXTURE_ARRAY(float4) uRWTextureArrays[]
    : register(REG_SLOT(t21000, u2048), space0);

VK_BINDING(0, 1) ConstantBuffer<SceneGlobals> uSceneGlobals
    : register(b0, space1);

#endif
