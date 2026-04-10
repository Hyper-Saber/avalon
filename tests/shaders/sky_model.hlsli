#ifndef AVALON_SKY_MODEL_HLSLI
#define AVALON_SKY_MODEL_HLSLI

#include "common.hlsli"
#include "noise.hlsli"

static const float3 kRayleighScat = float3(5.8e-6, 1.35e-5, 3.31e-5);
static const float3 kGroundColor = float3(0.02, 0.02, 0.02);
static const float3 kZenithColor = float3(0.05, 0.15, 0.4);
static const float3 kHorizonColor = float3(0.5, 0.7, 0.9);

float3 computeExtinction(float sunHeight) {
  float h = max(0.02, sunHeight);
  float opticalDepth = 1.0 / (h + 0.05);
  return exp(-kRayleighScat * 80000.0 * opticalDepth);
}

float4 computeSkyColor(float3 dir, float3 sunDir, float3 zenithColor,
                       float3 horizonColor, float3 groundColor) {
  float viewHeight = dir.y;
  float sunHeight = sunDir.y;
  float sunDot = dot(dir, sunDir);

  float3 extinction = computeExtinction(sunHeight);
  float3 lightBaseColor = uMainLight.colorIntensity.rgb * extinction;
  float lightIntensity = uMainLight.colorIntensity.a;

  float horizonBoost = smoothstep(0.1, -0.1, sunHeight);
  float3 sunRadiance = lerp(lightBaseColor, lightBaseColor * 2.5, horizonBoost);

  float dayStep = smoothstep(-0.15, 0.2, sunHeight);
  float sunsetStep = smoothstep(-0.25, 0.1, sunHeight);

  float3 currentZenith = lerp(float3(0.002, 0.005, 0.01), zenithColor, dayStep);
  float3 sunsetTint = float3(1.0, 0.45, 0.15) * sunsetStep;
  float3 currentHorizon =
      lerp(float3(0.01, 0.015, 0.02), lerp(sunsetTint, horizonColor, dayStep),
           sunsetStep);

  float backSideFactor = smoothstep(0.3, -1.0, sunDot);

  float azimuthDarkening = lerp(1.0, 0.6, backSideFactor * dayStep);

  float3 skyColor;
  if(viewHeight > 0.0) {
    float exponent = lerp(1.5, 5.0, 1.0 - dayStep);
    exponent *= lerp(1.0, 1.5, backSideFactor);

    float horizonGlow = pow(1.0 - viewHeight, exponent);

    skyColor = lerp(currentZenith, currentHorizon, horizonGlow);

    skyColor *= azimuthDarkening;

    float rayleighPhase = 0.75 * (1.0 + sunDot * sunDot);
    skyColor *= lerp(0.8, 1.2, rayleighPhase * dayStep);

    float mie = pow(max(0.0, sunDot), 16.0) * 0.2 * max(0.0, sunHeight + 0.2);
    skyColor += sunRadiance * mie;
  } else {
    skyColor = groundColor * sunsetStep;
  }

  float sunDiskMask = smoothstep(0.9997, 0.9999, sunDot);
  float3 diskFinal =
      (sunHeight > -0.05) ? (sunDiskMask * sunRadiance * lightIntensity) : 0.0;

  float sunGlowMask = pow(max(0.0, sunDot), 512.0);
  float3 glowFinal = sunGlowMask * sunRadiance * (lightIntensity * 0.6);

  float3 finalColor = skyColor + diskFinal + glowFinal;

  float noise = noise3d(dir * 2000.0);
  float luma = dot(finalColor, float3(0.2126, 0.7152, 0.0722));
  float noiseAmplitude = lerp(0.1, 1.0, smoothstep(0.0, 0.1, luma)) / 255.0;
  noise *= noiseAmplitude;

  return float4(max(0.0, finalColor + noise), 1.0);
}

#endif
