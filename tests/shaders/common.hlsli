#ifndef AVALON_COMMON_HLSLI
#define AVALON_COMMON_HLSLI

#include "constants.hlsli"
#include "standard_layout.hlsli"

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

#ifdef __COMPUTE_SHADER__
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
#endif

float4x4 loadDynamicMatrix(uint offset) {
  float4 c0 = asfloat(uDynamicSSBO.Load4(offset));
  float4 c1 = asfloat(uDynamicSSBO.Load4(offset + 16));
  float4 c2 = asfloat(uDynamicSSBO.Load4(offset + 32));
  float4 c3 = asfloat(uDynamicSSBO.Load4(offset + 48));

  return float4x4(c0.x, c1.x, c2.x, c3.x, c0.y, c1.y, c2.y, c3.y, c0.z, c1.z,
                  c2.z, c3.z, c0.w, c1.w, c2.w, c3.w);
}

float4x4 loadStaticMatrix(uint offset) {
  float4 c0 = asfloat(uStaticSSBO.Load4(offset));
  float4 c1 = asfloat(uStaticSSBO.Load4(offset + 16));
  float4 c2 = asfloat(uStaticSSBO.Load4(offset + 32));
  float4 c3 = asfloat(uStaticSSBO.Load4(offset + 48));

  return float4x4(c0.x, c1.x, c2.x, c3.x, c0.y, c1.y, c2.y, c3.y, c0.z, c1.z,
                  c2.z, c3.z, c0.w, c1.w, c2.w, c3.w);
}

InstanceData loadInstanceData(uint baseOffset, uint instanceID) {
  uint addr = baseOffset + (instanceID * 64);
  InstanceData d;
  uint4 g0 = uDynamicSSBO.Load4(addr);
  d.instanceID = g0.x;
  d.materialID = g0.y;
  d.posUVOffset = g0.z;
  d.attributesOffset = g0.w;

  uint4 g1 = uDynamicSSBO.Load4(addr + 16);
  d.indexOffset = g1.x;
  d.vertexCount = g1.y;
  d.indexCount = g1.z;
  d.modelOffset = g1.w;

  uint4 g2 = uDynamicSSBO.Load4(addr + 32);
  d.invModelOffset = g2.x;
  d.sdfTextureIndex = g2.y;
  d.sdfType = g2.z;
  d.alphaThreshold = asfloat(g2.w);

  d.sdfExtent = asfloat(uDynamicSSBO.Load3(addr + 48));
  return d;
}

uint loadVertexID(uint indexOffset, uint drawID) {
  return uIndicesSSBO.Load(indexOffset + drawID);
}

VertexPosUV loadVertexPosUV(uint vertexID, uint posUVOffset) {
  uint addr = posUVOffset + vertexID * kVertexPosUVSize;
  VertexPosUV res;
  res.position = asfloat(uPosUVSSBO.Load3(addr));
  res.uv = asfloat(uPosUVSSBO.Load2(addr + 12));
  return res;
}

VertexAttributes loadVertexAttributes(uint vertexID, uint attributesOffset) {
  uint addr = attributesOffset + vertexID * kVertexAttributesSize;
  VertexAttributes res;
  res.normal = asfloat(uAttributesSSBO.Load3(addr));
  res.color = asfloat(uAttributesSSBO.Load3(addr + 12));
  return res;
}

float4 calculateWorldPosition(uint modelOffset, float3 localPosition) {
  return mul(loadDynamicMatrix(modelOffset), float4(localPosition, 1.0));
}

float3 calculateWorldNormal(uint invModelOffset, float3 localNormal) {
  float4x4 invModel = loadDynamicMatrix(invModelOffset);
  return normalize(mul((float3x3)transpose(invModel), localNormal));
}

float4 calculateClipPosition(float4 worldPos) {
  return mul(uCamera.viewProjection, worldPos);
}

float4 ndcToLocal(uint invModelOffset, float4 ndcPosition) {
  float4 worldPosH = mul(uCamera.invViewProjection, ndcPosition);
  float4 worldPos = worldPosH / worldPosH.w;
  return mul(loadDynamicMatrix(invModelOffset), worldPos);
}

#ifdef STANDARD_PUSH
InstanceData loadInstanceData(uint instanceID) {
  return loadInstanceData(push.instanceBufferOffset, instanceID);
}
#endif

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
