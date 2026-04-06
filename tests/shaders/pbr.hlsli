#ifndef AVALON_PBR_HLSLI
#define AVALON_PBR_HLSLI
#include "common.hlsli"

float distributionGGX(float3 N, float3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  float nom = a2;
  float denom = NdotH2 * (a2 - 1.0) + 1.0;
  denom = kPi * denom * denom;

  return nom / max(denom, 1e-8);
}

float geometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = r * r / 8.0;

  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k + kEpsilon;
  return nom / denom;
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
  float ggx2 = geometrySchlickGGX(NdotV, roughness);
  float ggx1 = geometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

float3 FresnelSchlick(float NdotV, float3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
}

#endif
