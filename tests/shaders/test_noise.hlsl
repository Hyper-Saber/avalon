#include "common.hlsli"
#include "noise.hlsli"
#include "screen_space.hlsli"

VSOutput VsMain(uint vertexID : SV_VertexID) {
  return FullscreenBase(vertexID);
}

float4 FsMain(VSOutput input) : SV_TARGET {
  float2 uv = input.uv * 5.0;
  float time = uTime * 0.2;
  float2 motion = float2(noise1d(time), noise1d(time + 100.0));
  float n = fbm2d(uv + motion, 6.0);
  float3 color = float3(n * 0.5 + 0.5, n * 0.5 + 0.5, n * 0.5 + 0.55);
  color *= smoothstep(1.5, 0.0, length(input.uv - 0.5));
  return float4(color, 1.0);
}
