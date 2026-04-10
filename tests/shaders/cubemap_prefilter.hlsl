#include "constants.hlsli"
struct CustomPush {
  uint srcIndex;
  uint dstIndex;
  uint samplerIndex;
  uint sampleCount;
  uint size;
  float invSize;
  float roughness;
  uint paddings[kPushConstantFloatSize - 7];
};

#define CUSTOM_PUSH_TYPE CustomPush
#include "common.hlsli"
#include "pbr.hlsli"

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

float3 getCubeMapDirection(float2 uv, uint faceIndex) {
  float2 st = uv * 2.0 - 1.0;

  float3 f = kFaceForwards[faceIndex];
  float3 r = kFaceRights[faceIndex];
  float3 u = kFaceUps[faceIndex];

  return normalize(f + st.x * r + st.y * u);
}

[numthreads(8, 8, 1)] void CsMain(uint3 dispatchId : SV_DispatchThreadID,
                                  uint groupIndex : SV_GroupIndex) {
  uint size = push.size;

  if(dispatchId.x >= size || dispatchId.y >= size)
    return;

  float2 uv = float2(dispatchId.xy + 0.5) * push.invSize;
  float3 N = getCubeMapDirection(uv, dispatchId.z);
  float3 R = N;
  float3 V = R;

  float3 prefilteredColor = 0.0;
  float totalWeight = 0.0;

  uint srcIndex = push.srcIndex;
  uint dstIndex = push.dstIndex;
  uint samplerIndex = push.samplerIndex;
  uint sampleCount = push.sampleCount;
  float roughness = push.roughness;

  for(uint i = 0; i < sampleCount; i++) {
    float2 Xi = hammersley(i, sampleCount);
    float3 H = importanceSampleGGX(Xi, N, roughness);
    float3 L = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(dot(N, L), 0.0);
    if(NdotL > 0.0) {
      prefilteredColor +=
          sampleCubeLod(srcIndex, samplerIndex, L, 0.0).rgb * NdotL;
      totalWeight += NdotL;
    }
  }

  prefilteredColor /= totalWeight;

  writeRWTextureArray(push.dstIndex, dispatchId, float4(prefilteredColor, 1.0));
}
