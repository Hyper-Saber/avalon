#include "common.hlsli"

struct VsOputput {
  float4 position : SV_Position;
  float3 viewDir : TEXCOORD0;
  // uint viewId : SV_RenderTargetArrayIndex;
};

VsOputput VsMain(uint vertexId : SV_VertexID, uint viewId : SV_ViewID) {
  VsOputput output;

  static const float2 uvTable[3] = {float2(0, 0), float2(0, 2), float2(2, 0)};

  float2 uv = uvTable[vertexId];
  output.position = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, 0, 1.0);

  float4x4 invView = uProbes[0].captureViews[viewId];

  float3 dir = mul((float3x3)invView, float3(output.position.xy, -1.0));
  output.viewDir = normalize(dir);
  return output;
}

float4 FsMain(VsOputput input) : SV_Target {
  return float4(input.viewDir * 0.5 + 0.5, 1.0);
}
