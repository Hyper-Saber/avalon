#include "constants.hlsli"
struct CustomPush {
  uint brdfLutIndex;
  uint sampleCount;
  float invSize;
  uint paddings[kPushConstantFloatSize - 3];
};

#define CUSTOM_PUSH_TYPE CustomPush

#include "common.hlsli"
#include "pbr.hlsli"

[numthreads(8, 8, 1)] void CsMain(uint3 dispatchId : SV_DispatchThreadID) {
  float2 uv = float2(dispatchId.xy + 0.5) * push.invSize;

  float NdotV = uv.x;
  float roughness = uv.y;

  float3 V;
  V.x = sqrt(1.0 - NdotV * NdotV);
  V.y = 0.0;
  V.z = NdotV;

  float A = 0.0;
  float B = 0.0;

  float3 N = float3(0.0, 0.0, 1.0);
  uint sampleCount = push.sampleCount;

  for(uint i = 0; i < sampleCount; i++) {
    float2 Xi = hammersley(i, sampleCount);
    float3 H = importanceSampleGGX(Xi, N, roughness);

    float VdotH = max(dot(V, H), 0.0);

    float3 L = normalize(2.0 * VdotH * H - V);

    float NdotL = max(L.z, 0.0);
    float NdotH = max(H.z, 0.0);

    if(NdotL > 0.0) {
      float G_Vis = visibilitySmithJointGGX(NdotV, NdotL, roughness);
      G_Vis *= 4.0 * VdotH * NdotL / NdotH;

      float Fc = pow(1.0 - VdotH, 5.0);

      A += (1.0 - Fc) * G_Vis;
      B += Fc * G_Vis;
    }
  }
  float Eavg = A + B;

  writeRWTexture2d(push.brdfLutIndex, dispatchId.xy,
                   float4(A / sampleCount, B / sampleCount, Eavg, 1.0));
}
