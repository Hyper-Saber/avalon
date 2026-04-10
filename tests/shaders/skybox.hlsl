#include "common.hlsli"
#include "screen_space.hlsli"
#include "sky_model.hlsli"

VsOutput VsMain(uint vertexID : SV_VertexID) {
  // return FullscreenBase(vertexID);
  return fullscreenSkybox(vertexID, uCamera.invProjection, uCamera.invView);
}

float4 FsMain(VsOutput input) : SV_Target {
  float3 dir = normalize(input.viewDir);
  float3 sunDir = normalize(-uMainLight.posDir.xyz);

  float4 finalColor =
      computeSkyColor(dir, sunDir, kZenithColor, kHorizonColor, kGroundColor);

  return finalColor;
}
