#include "constants.hlsli"
struct CustomPush {
  uint brdfLutIndex;
  float invSize;
  uint paddings[kPushConstantFloatSize - 2];
};

#define CUSTOM_PUSH_TYPE CustomPush

#include "common.hlsli"
#include "pbr.hlsli"

[numthreads(8, 8, 1)] void CsMain(uint3 dispatchId : SV_DispatchThreadID) {
  float2 uv = float2(dispatchId.xy + 0.5) * push.invSize;

  float NdotV = uv.x;
  float4 brdf = loadRWTexture2d(push.brdfLutIndex, dispatchId.xy);

  brdf.b = 2 * (brdf.r + brdf.g) / uv.x;

  writeRWTexture2d(push.brdfLutIndex, dispatchId.xy, brdf);
}
