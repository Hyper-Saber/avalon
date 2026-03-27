#include "common.hlsli"
#include "noise.hlsli"

struct VSOutput {
  float4 clipPos : SV_POSITION;
  VK_LOCATION(0) float2 uv : TEXCOORD;
};

VSOutput VsMain(uint vertexID : SV_VertexID) {
  VSOutput output;

  output.uv = float2((vertexID << 1) & 2, vertexID & 2);
  output.clipPos = float4(output.uv * 2 - 1, 0, 1);
  output.clipPos.y = -output.clipPos.y;
  return output;
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
