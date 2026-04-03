#include "common.hlsli"
#include "screen_space.hlsli"

VsOutput VsMain(uint vertexID : SV_VertexID) {
  // return FullscreenBase(vertexID);
  return FullscreenSkybox(vertexID, uCamera.invProjection, uCamera.invView);
}

static const float3 kZenithColor = float3(0.05, 0.15, 0.4);
static const float3 kHorizonColor = float3(0.5, 0.7, 0.9);
static const float3 kGroundColor = float3(0.1, 0.1, 0.1);

float4 FsMain(VsOutput input) : SV_Target {
  float3 dir = normalize(input.viewDir);
  float3 sunDir = normalize(-uMainLight.posDir.xyz);
  float viewHeight = dir.y;
  float sunHeight = max(0.0, sunDir.y);

  float3 currentZenith =
      lerp(float3(0.01, 0.02, 0.05), kZenithColor, sunHeight);
  float3 skyColor = (viewHeight > 0.0) ? lerp(kHorizonColor, currentZenith,
                                              pow(viewHeight, 0.2))
                                       : kGroundColor;

  float sunAngularRadius = 0.01;
  float sunDot = dot(dir, sunDir);
  float sunDisk = smoothstep(0.9998, 1.0, sunDot);
  float sunGlow = pow(max(0.0, sunDot), 256.0);
  float3 lightBaseColor = uMainLight.colorIntensity.rgb;
  float lightIntensity = uMainLight.colorIntensity.a;
  float3 diskFinal = sunDisk * lightBaseColor * lightIntensity;
  float3 glowColor = lerp(float3(1.0, 0.2, 0.0), lightBaseColor, sunHeight);
  float3 glowFinal = sunGlow * glowColor * (lightIntensity * 0.5);
  float3 finalColor = skyColor + diskFinal + glowFinal;
  return float4(finalColor, 1);
}
