#include "common.hlsli"
#include "screen_space.hlsli"

VsOutput VsMain(uint vertexID : SV_VertexID) {
  // return FullscreenBase(vertexID);
  return FullscreenSkybox(vertexID, uCamera.invProjection, uCamera.invView);
}

static const float3 kZenithColor = float3(0.05, 0.15, 0.4);
static const float3 kHorizonColor = float3(0.5, 0.7, 0.9);
static const float3 kGroundColor = float3(0.1, 0.1, 0.1);
static const float3 kSunColor = float3(1.0, 0.9, 0.7) * 5.0;

float4 FsMain(VsOutput input) : SV_Target {

  float3 dir = normalize(input.viewDir);
  float height = dir.y;
  float3 skyColor;

  if(height > 0.0) {
    float t = pow(height, 0.5);
    skyColor = lerp(kHorizonColor, kZenithColor, t);
  } else {
    float t = pow(saturate(-height), 0.2);
    skyColor = lerp(kHorizonColor, kGroundColor, t);
  }

  float3 sunDir = normalize(-uMainLight.posDir.xyz);
  float sunDot = saturate(dot(dir, sunDir));
  float sunMask = pow(sunDot, 1000.0);
  float3 finalColor = skyColor + (sunMask * kSunColor);
  return float4(finalColor, 1);
}
