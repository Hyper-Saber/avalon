#include "constants.hlsli"
struct CustomPush {
  uint envMap;
  uint sampler;
  uint outputOffset;
  uint weightOffset;
  uint sampleLevel;
  float invSize;
  float paddings[kPushConstantFloatSize - 6];
};

#define CUSTOM_PUSH_TYPE CustomPush
#include "common.hlsli"
#include "pbr.hlsli"

groupshared float3 g_sharedSH[64][9];
groupshared float g_sharedWeight[64];

[numthreads(8, 8, 1)] void CsMain(uint3 dispatchId : SV_DispatchThreadID,
                                  uint groupIndex : SV_GroupIndex) {
  uint face = dispatchId.z;

  float2 uv = (dispatchId.xy + 0.5) * push.invSize;

  float3 dir = getCubemapDirection(uv, dispatchId.z);

  float weight = 1.0 + uv.x * uv.x + uv.y * uv.y;
  weight = 4.0 / (sqrt(weight) * weight);

  float3 color =
      sampleCubeLod(push.envMap, push.sampler, dir, push.sampleLevel).rgb;

  float basis[9];
  computeSHBasis(dir, basis);

  g_sharedWeight[groupIndex] = weight;
  for(int i = 0; i < 9; i++) {
    g_sharedSH[groupIndex][i] = color * basis[i] * weight;
  }

  GroupMemoryBarrierWithGroupSync();

  for(uint s = 32; s > 0; s >>= 1) {
    if(groupIndex < s) {
      g_sharedWeight[groupIndex] += g_sharedWeight[groupIndex + s];
      for(int j = 0; j < 9; j++) {
        g_sharedSH[groupIndex][j] += g_sharedSH[groupIndex + s][j];
      }
    }
    GroupMemoryBarrierWithGroupSync();
  }

  if(groupIndex == 0) {
    uint baseAddr = push.outputOffset;
    ATOMIC_ADD_FLOAT(uGeneralSSBO, push.weightOffset, g_sharedWeight[0]);
    [[unroll]] for(int i = 0; i < 9; i++) {
      float3 finalVal = g_sharedSH[0][i];
      uint addr = baseAddr + i * 16;

      ATOMIC_ADD_FLOAT(uGeneralSSBO, addr + 0, finalVal.r);
      ATOMIC_ADD_FLOAT(uGeneralSSBO, addr + 4, finalVal.g);
      ATOMIC_ADD_FLOAT(uGeneralSSBO, addr + 8, finalVal.b);
    }
  }
}
