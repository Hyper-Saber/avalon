#include "common.hlsli"

VK_PUSH_CONSTANT ModelData push;

struct VSInput {
  VK_LOCATION(0) float3 position : POSITION;
  VK_LOCATION(1) float2 uv : TEXCOORD;
  VK_LOCATION(2) float4 color : COLOR;
};

struct VSOutput {
  float4 clipPos : SV_POSITION;
  VK_LOCATION(0) float4 color : COLOR;
  VK_LOCATION(1) float2 uv : TEXCOORD;
};

VSOutput VsMain(VSInput input) {
  VSOutput output;
  float4 worldPos = mul(push.model, float4(input.position, 1.0f));
  float4 viewPos = mul(uSceneGlobals.view, worldPos);
  output.clipPos = mul(uSceneGlobals.projection, viewPos);
  output.color = input.color;
  output.uv = input.uv;

  return output;
}

float4 FsMain(VSOutput input) : SV_TARGET {
  // return float4(1, 1, 1, 1);
  return input.color;
}
