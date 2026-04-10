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

[numthreads(8, 8, 1)] void CsMain(uint3 dispatchId : SV_DispatchThreadID,
                                  uint groupIndex : SV_GroupIndex) {
  uint size = push.size;

  if(dispatchId.x >= size || dispatchId.y >= size)
    return;

  float2 uv = float2(dispatchId.xy + 0.5) * push.invSize;
  float3 N = getCubemapDirection(uv, dispatchId.z);
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
