#include "common.hlsli"

VK_BINDING(0, 1) Texture2D<float> uHeightMap : register(t1, space1);
VK_BINDING(1, 1) SamplerState uLinearClampSampler : register(s2, space1);

struct PushConstant {
  float strength;
  float padding[3];
};

VK_PUSH_CONSTANT PushConstant push;

struct VSOutput {
  float4 clipPos : SV_POSITION;
  VK_LOCATION(0) float2 uv : TEXCOORD;
};

float getHeight(float2 uv, float2 offset) {
  return uHeightMap.Sample(uLinearClampSampler, uv + offset * uResolution.zw).r;
}

VSOutput VsMain(uint vertexID : SV_VertexID) {
  VSOutput output;

  output.uv = float2((vertexID << 1) & 2, vertexID & 2);
  output.clipPos = float4(output.uv * 2 - 1, 0, 1);
  output.clipPos.y = -output.clipPos.y;
  return output;
}

float4 FsMain(VSOutput input) : SV_TARGET {
  float tl = getHeight(input.uv, float2(-1, -1));
  float t = getHeight(input.uv, float2(0, -1));
  float tr = getHeight(input.uv, float2(1, -1));
  float l = getHeight(input.uv, float2(-1, 0));
  float r = getHeight(input.uv, float2(1, 0));
  float bl = getHeight(input.uv, float2(-1, 1));
  float b = getHeight(input.uv, float2(0, 1));
  float br = getHeight(input.uv, float2(1, 1));

  float dx = (tr + 2.0 * r + br) - (tl + 2.0 * l + bl);
  float dy = (bl + 2.0 * b + br) - (tl + 2.0 * t + tr);

  float3 normal =
      normalize(float3(-dx * push.strength, -dy * push.strength, 1.0));

  return float4(normal * 0.5 + 0.5, 1.0);
}
